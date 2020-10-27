#pragma once

#include "Match/MatchBase.h"

#include "Application/API/Events/NetworkEvents.h"

class MatchServer : public MatchBase
{
public:
	MatchServer() = default;
	~MatchServer();

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	virtual void SpawnFlag();

	virtual bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event) override final;
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) override final;

private:
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
};