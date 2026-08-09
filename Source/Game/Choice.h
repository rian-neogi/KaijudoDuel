#pragma once

#include "BattleZone.h"

class Choice
{
public:
	static const int AI_NO_PREFERENCE = -4;

	std::string mInfotext;
	int mButtonCount;
	int mValidRef;
	int mActionRef;
	int mAiPreferredSelection;

	bool mIsCopy;

	Choice();
	Choice(std::string info, int skip, int vr, int ar, int aiPreferredSelection);
	~Choice();

	int callvalid(int cid, int sid);
	void callaction(int cid, int sid);
	void copyFrom(Choice* c);
};

