#pragma once

#include "Match/MatchBase.h"

class MatchClient : public MatchBase
{
public:
	MatchClient() = default;
	~MatchClient() = default;

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	virtual bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event) override final;
};