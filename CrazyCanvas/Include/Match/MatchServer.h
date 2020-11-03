#pragma once

#include "Match/MatchBase.h"

#include "Application/API/Events/NetworkEvents.h"
#include "Events/MatchEvents.h"

#include "ECS/Entity.h"

class MatchServer : public MatchBase
{
public:
	MatchServer() = default;
	~MatchServer();

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	void MatchStart();
	void MatchBegin();

	void SpawnFlag();
	void SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient);
	void DeleteGameLevelObject(LambdaEngine::Entity entity);
	
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;
	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) override final;

private:
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
	bool OnFlagDelivered(const FlagDeliveredEvent& event);

private:
	uint8 m_NextTeamIndex = 0;
};