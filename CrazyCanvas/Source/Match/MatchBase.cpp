#include "Match/MatchBase.h"

bool MatchBase::Init()
{
	if (!InitInternal())
	{
		return false;
	}

	return true;
}
