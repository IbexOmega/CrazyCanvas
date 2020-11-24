#pragma once

#include "Match/MatchBase.h"

#include "Application/API/Events/NetworkEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/MatchEvents.h"
#include "Events/PacketEvents.h"

#include "ECS/Entity.h"

#include "Lobby/Player.h"

typedef std::pair<LambdaEngine::Entity, float32> PlayerRespawnTimer;

class MatchServer : public MatchBase
{
	struct PlayerKillDesc
	{
		LambdaEngine::Entity PlayerToKill	= UINT32_MAX;
		bool RespawnFlagIfCarried			= false;
	};

public:
	MatchServer();
	~MatchServer();

	virtual void KillPlaneCallback(LambdaEngine::Entity killPlaneEntity, LambdaEngine::Entity otherEntity) override final;
	
protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
	virtual void FixedTickInternal(LambdaEngine::Timestamp deltaTime) override final;

	virtual bool ResetMatchInternal() override final;

	virtual void BeginLoading() override final;
	virtual void MatchStart() override final;
	void MatchBegin();

	void SpawnPlayer(const Player& player);

	void SpawnFlag(uint8 teamIndex);
	void DeleteGameLevelObject(LambdaEngine::Entity entity);
	
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;

private:
	bool OnPlayerLeft(const PlayerLeftEvent& event);
	bool OnFlagDelivered(const FlagDeliveredEvent& event);
	bool OnFlagRespawn(const FlagRespawnEvent& event);

	void RespawnPlayer(LambdaEngine::Entity entity);
	void DoKillPlayer(LambdaEngine::Entity playerEntity, bool respawnFlag);

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	void InternalKillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity, bool respawnFlagIfCarried);

	void InternalSetScore(uint8 team, uint32 score);

public:
	static void KillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity, bool respawnFlagIfCarried);

	bool CreateFlagSpawnProperties(uint8 teamIndex, glm::vec3& position);

private:
	LambdaEngine::SpinLock m_PlayersToKillLock;
	LambdaEngine::TArray<PlayerKillDesc> m_PlayersToKill;

	LambdaEngine::SpinLock m_PlayersToRespawnLock;
	//LambdaEngine::TQueue<PlayerTimers> m_PlayersToRespawn;
	LambdaEngine::TArray<PlayerRespawnTimer> m_PlayersToRespawn;

	bool m_ShouldBeginMatch = false;
};