#pragma once

#include "Match/MatchBase.h"

class MatchClient : public MatchBase
{
public:
	MatchClient() = default;
	~MatchClient() = default;

protected:
	virtual bool InitInternal() override final;

};