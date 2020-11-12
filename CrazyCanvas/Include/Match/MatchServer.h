#pragma once

#include "Match/MatchBase.h"

#include "Application/API/Events/NetworkEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/MatchEvents.h"
#include "Events/PacketEvents.h"

#include "ECS/Entity.h"

#include "Lobby/Player.h"

class MatchServer : public MatchBase
{
public:
	MatchServer() = default;
	~MatchServer();

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	virtual void KillPlayer(LambdaEngine::Entity playerEntity) override final;

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
	virtual void FixedTickInternal(LambdaEngine::Timestamp deltaTime) override final;

	void MatchStart();
	void MatchBegin();

	void SpawnPlayer(const Player& player);

	void SpawnFlag();
	void DeleteGameLevelObject(LambdaEngine::Entity entity);
	
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;

private:
	void BeginLoading();
	bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	bool OnFlagDelivered(const FlagDeliveredEvent& event);
	bool OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event);

	void KillPlayerInternal(LambdaEngine::Entity playerEntity);

private:
	LambdaEngine::SpinLock m_PlayersToKillLock;
	LambdaEngine::TArray<LambdaEngine::Entity> m_PlayersToKill;
	uint8 m_NextTeamIndex = 0;
	bool m_ShouldBeginMatch = false;
};