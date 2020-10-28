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

	virtual bool OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event) override final;
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) override final;
};