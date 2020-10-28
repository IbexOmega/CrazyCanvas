#pragma once

#include "Match/MatchBase.h"

#include "Application/API/Events/NetworkEvents.h"
#include "Events/MatchEvents.h"

class MatchServer : public MatchBase
{
public:
	MatchServer() = default;
	~MatchServer();

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	void SpawnFlag();
	void SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient);
	
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) override final;

private:
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
	bool OnFlagDelivered(const OnFlagDeliveredEvent& event);

private:
	uint8 m_NextTeamIndex = 0;
};