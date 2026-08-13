#include "LuaTrace.h"

#include "Game/Duel.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <thread>

namespace
{
	const char* TRACE_PATH = "Logs/lua-trace.log";
	const char* PREVIOUS_TRACE_PATH = "Logs/lua-trace.previous.log";
	const size_t MAX_TRACE_SIZE = 16 * 1024 * 1024;

	struct TraceState
	{
		bool enabled = false;
		FILE* file = NULL;
		size_t bytesWritten = 0;
		unsigned long long sequence = 0;
		std::mutex mutex;

		~TraceState()
		{
			if (file != NULL)
				std::fclose(file);
		}
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
		return traceState().enabled && (ActiveDuel == NULL || !ActiveDuel->mIsSimulation);
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

	void openTrace(TraceState& state, bool preserveCurrent)
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
	}

	void writeTrace(const std::string& text)
	{
		TraceState& state = traceState();
		if (!state.enabled)
			return;
		std::lock_guard<std::mutex> lock(state.mutex);
		if (state.file == NULL)
			return;
		if (state.bytesWritten + text.size() + 96 > MAX_TRACE_SIZE)
			openTrace(state, true);
		std::ostringstream prefix;
		prefix << '[' << ++state.sequence << "] [thread " << std::this_thread::get_id() << "] ";
		std::string line = prefix.str() + text + '\n';
		std::fwrite(line.data(), 1, line.size(), state.file);
		std::fflush(state.file);
		state.bytesWritten += line.size();
	}
}

void LuaTrace::setEnabled(bool enabled)
{
	TraceState& state = traceState();
	std::lock_guard<std::mutex> lock(state.mutex);
	if (state.enabled == enabled)
		return;
	state.enabled = enabled;
	if (!enabled)
	{
		if (state.file != NULL) std::fclose(state.file);
		state.file = NULL;
		return;
	}
	if (mkdir("Logs", 0755) != 0 && errno != EEXIST)
	{
		state.enabled = false;
		return;
	}
	openTrace(state, true);
}

bool LuaTrace::isEnabled()
{
	return traceState().enabled;
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
