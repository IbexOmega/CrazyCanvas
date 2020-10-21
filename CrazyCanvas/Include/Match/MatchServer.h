#pragma once

#include "Match/MatchBase.h"

class MatchServer : public MatchBase
{
public:
	MatchServer() = default;
	~MatchServer() = default;

protected:
	virtual bool InitInternal() override final;

};