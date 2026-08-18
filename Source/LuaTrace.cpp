#include "LuaTrace.h"

#include "Game/Duel.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <utility>

namespace
{
	const char* TRACE_PATH = "Logs/lua-trace.log";
	const char* PREVIOUS_TRACE_PATH = "Logs/lua-trace.previous.log";
	const size_t MAX_TRACE_SIZE = 16 * 1024 * 1024;
	const size_t MAX_PENDING_BYTES = 8 * 1024 * 1024;
	const size_t TRACE_BATCH_BYTES = 64 * 1024;
	const int TRACE_BATCH_DELAY_MS = 25;
	thread_local int suppressionDepth = 0;

	struct TraceState
	{
		std::atomic<bool> enabled;
		FILE* file = NULL;
		size_t bytesWritten = 0;
		std::atomic<unsigned long long> sequence;
		std::deque<std::string> pendingLines;
		size_t pendingBytes = 0;
		unsigned long long droppedLines = 0;
		bool stopping = false;
		std::mutex queueMutex;
		std::condition_variable wakeWriter;
		std::thread writer;
		std::mutex lifecycleMutex;

		TraceState() : enabled(false), sequence(0) {}
		~TraceState();
	};

	TraceState& traceState()
	{
		static TraceState state;
		return state;
	}

	bool traceCurrentDuel()
	{
		// MCTS can execute thousands of callbacks for positions that are never
		// committed to the real duel. Logging those callbacks consumes most of
		// the search budget and makes the UI contend with continuous file I/O.
		return suppressionDepth == 0 &&
			traceState().enabled.load(std::memory_order_relaxed) &&
			(ActiveDuel == NULL || !ActiveDuel->mIsSimulation);
	}

	std::string escape(const char* value, size_t length)
	{
		std::ostringstream output;
		output << '"';
		size_t limit = length > 240 ? 240 : length;
		for (size_t i = 0; i < limit; i++)
		{
			unsigned char c = static_cast<unsigned char>(value[i]);
			if (c == '\\' || c == '"') output << '\\' << static_cast<char>(c);
			else if (c == '\n') output << "\\n";
			else if (c == '\r') output << "\\r";
			else if (c == '\t') output << "\\t";
			else if (c >= 32 && c < 127) output << static_cast<char>(c);
			else output << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << std::dec;
		}
		if (length > limit) output << "...";
		output << '"';
		return output.str();
	}

	std::string luaValue(lua_State* state, int index)
	{
		switch (lua_type(state, index))
		{
		case LUA_TNIL: return "nil";
		case LUA_TBOOLEAN: return lua_toboolean(state, index) ? "true" : "false";
		case LUA_TNUMBER:
		{
			std::ostringstream output;
			if (lua_isinteger(state, index)) output << lua_tointeger(state, index);
			else output << lua_tonumber(state, index);
			return output.str();
		}
		case LUA_TSTRING:
		{
			size_t length = 0;
			const char* value = lua_tolstring(state, index, &length);
			return escape(value == NULL ? "" : value, length);
		}
		default:
			return std::string("<") + lua_typename(state, lua_type(state, index)) + ">";
		}
	}

	std::string luaValues(lua_State* state, int first, int count)
	{
		std::ostringstream output;
		output << '(';
		for (int i = 0; i < count; i++)
		{
			if (i != 0) output << ", ";
			output << luaValue(state, first + i);
		}
		output << ')';
		return output.str();
	}

	std::string messageValues(const Message& message)
	{
		std::ostringstream output;
		output << '{';
		bool first = true;
		for (std::map<std::string, std::string>::const_iterator value = message.map.begin();
			value != message.map.end(); value++)
		{
			if (!first) output << ", ";
			first = false;
			output << value->first << '=' << escape(value->second.c_str(), value->second.size());
		}
		output << '}';
		return output.str();
	}

	bool openTrace(TraceState& state, bool preserveCurrent)
	{
		if (state.file != NULL)
		{
			std::fclose(state.file);
			state.file = NULL;
		}
		if (preserveCurrent)
		{
			std::remove(PREVIOUS_TRACE_PATH);
			std::rename(TRACE_PATH, PREVIOUS_TRACE_PATH);
		}
		state.file = std::fopen(TRACE_PATH, "w");
		state.bytesWritten = 0;
		if (state.file == NULL)
			return false;
		std::setvbuf(state.file, NULL, _IOFBF, 64 * 1024);
		return true;
	}

	void writeLine(TraceState& state, const std::string& line)
	{
		if (state.file == NULL)
			return;
		if (state.bytesWritten + line.size() > MAX_TRACE_SIZE &&
			!openTrace(state, true))
			return;
		std::fwrite(line.data(), 1, line.size(), state.file);
		state.bytesWritten += line.size();
	}

	std::string prefixedLine(TraceState& state, const std::string& text)
	{
		std::ostringstream prefix;
		prefix << '[' << state.sequence.fetch_add(1, std::memory_order_relaxed) + 1 <<
			"] [thread " << std::this_thread::get_id() << "] ";
		return prefix.str() + text + '\n';
	}

	void traceWriter(TraceState* state)
	{
		for (;;)
		{
			std::deque<std::string> lines;
			unsigned long long droppedLines = 0;
			{
				std::unique_lock<std::mutex> lock(state->queueMutex);
				state->wakeWriter.wait(lock, [state]
				{
					return state->stopping || !state->pendingLines.empty();
				});
				if (!state->stopping && state->pendingBytes < TRACE_BATCH_BYTES)
				{
					state->wakeWriter.wait_for(lock,
						std::chrono::milliseconds(TRACE_BATCH_DELAY_MS), [state]
						{
							return state->stopping ||
								state->pendingBytes >= TRACE_BATCH_BYTES;
						});
				}
				if (state->pendingLines.empty() && state->stopping)
					break;
				lines.swap(state->pendingLines);
				state->pendingBytes = 0;
				droppedLines = state->droppedLines;
				state->droppedLines = 0;
			}

			for (std::deque<std::string>::const_iterator line = lines.begin();
				line != lines.end(); line++)
				writeLine(*state, *line);
			if (droppedLines != 0)
			{
				std::ostringstream warning;
				warning << "trace writer dropped " << droppedLines <<
					" lines because its queue was full";
				writeLine(*state, prefixedLine(*state, warning.str()));
			}
			if (state->file != NULL)
				std::fflush(state->file);
		}
		if (state->file != NULL)
			std::fflush(state->file);
	}

	void stopTrace(TraceState& state)
	{
		state.enabled.store(false, std::memory_order_release);
		{
			std::lock_guard<std::mutex> lock(state.queueMutex);
			state.stopping = true;
		}
		state.wakeWriter.notify_one();
		if (state.writer.joinable())
			state.writer.join();
		if (state.file != NULL)
		{
			std::fclose(state.file);
			state.file = NULL;
		}
		std::lock_guard<std::mutex> lock(state.queueMutex);
		state.pendingLines.clear();
		state.pendingBytes = 0;
		state.droppedLines = 0;
		state.stopping = false;
	}

	TraceState::~TraceState()
	{
		stopTrace(*this);
	}

	void writeTrace(const std::string& text)
	{
		TraceState& state = traceState();
		if (!state.enabled.load(std::memory_order_relaxed))
			return;
		std::string line = prefixedLine(state, text);
		bool notify = false;
		{
			std::lock_guard<std::mutex> lock(state.queueMutex);
			if (!state.enabled.load(std::memory_order_relaxed) || state.stopping)
				return;
			if (state.pendingBytes + line.size() > MAX_PENDING_BYTES)
			{
				state.droppedLines++;
				return;
			}
			notify = state.pendingLines.empty() ||
				(state.pendingBytes < TRACE_BATCH_BYTES &&
				state.pendingBytes + line.size() >= TRACE_BATCH_BYTES);
			state.pendingBytes += line.size();
			state.pendingLines.push_back(std::move(line));
		}
		if (notify)
			state.wakeWriter.notify_one();
	}
}

LuaTrace::ScopedSuppression::ScopedSuppression()
{
	suppressionDepth++;
}

LuaTrace::ScopedSuppression::~ScopedSuppression()
{
	suppressionDepth--;
}

void LuaTrace::setEnabled(bool enabled)
{
	TraceState& state = traceState();
	std::lock_guard<std::mutex> lock(state.lifecycleMutex);
	if (state.enabled.load(std::memory_order_acquire) == enabled)
		return;
	if (!enabled)
	{
		stopTrace(state);
		return;
	}
	if (mkdir("Logs", 0755) != 0 && errno != EEXIST)
		return;
	if (!openTrace(state, true))
		return;
	{
		std::lock_guard<std::mutex> queueLock(state.queueMutex);
		state.stopping = false;
	}
	try
	{
		state.writer = std::thread(traceWriter, &state);
	}
	catch (...)
	{
		std::fclose(state.file);
		state.file = NULL;
		return;
	}
	state.enabled.store(true, std::memory_order_release);
}

bool LuaTrace::isEnabled()
{
	return traceState().enabled.load(std::memory_order_acquire);
}

void LuaTrace::logCallback(const char* direction, const char* callback, const std::string& subject,
	int cardId, const Message& message, const char* error)
{
	if (!traceCurrentDuel()) return;
	std::ostringstream output;
	output << direction << ' ' << callback << " card=" << escape(subject.c_str(), subject.size())
		<< " uid=" << cardId << " message=" << messageValues(message);
	if (error != NULL) output << " error=" << escape(error, std::strlen(error));
	writeTrace(output.str());
}

void LuaTrace::logBridgeCall(lua_State* state, const char* function, int argumentCount)
{
	if (!traceCurrentDuel()) return;
	writeTrace(std::string("lua -> engine ") + function + " args=" + luaValues(state, 1, argumentCount));
}

void LuaTrace::logBridgeReturn(lua_State* state, const char* function, int resultCount)
{
	if (!traceCurrentDuel()) return;
	writeTrace(std::string("engine -> lua ") + function + " returns=" + luaValues(state, 1, resultCount));
}

void LuaTrace::logBridgeError(lua_State* state, const char* function)
{
	if (!traceCurrentDuel()) return;
	const char* error = lua_tostring(state, -1);
	writeTrace(std::string("engine -> lua ") + function + " error=" +
		escape(error == NULL ? "unknown error" : error, error == NULL ? 13 : std::strlen(error)));
}
