#pragma once

#include "Match/MatchBase.h"

#include "Multiplayer/Packet/PacketStartGame.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"

#include "Application/API/Events/NetworkEvents.h"
#include "Events/MatchEvents.h"

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

	void CreatePlayer(const Player& player);

	void SpawnFlag();
	//void SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient);
	void DeleteGameLevelObject(LambdaEngine::Entity entity);
	
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;

private:
	bool OnPacketStartGameReceived(const PacketReceivedEvent<PacketStartGame>& event);
	//bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
	bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	bool OnFlagDelivered(const FlagDeliveredEvent& event);

	void KillPlayerInternal(LambdaEngine::Entity playerEntity);

private:
	LambdaEngine::SpinLock m_PlayersToKillLock;
	LambdaEngine::TArray<LambdaEngine::Entity> m_PlayersToKill;
	uint8 m_NextTeamIndex = 0;
};