#pragma once
#include "ECS/System.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Multiplayer/PacketPlayerActionComponent.h"
#include "ECS/Components/Multiplayer/PacketPlayerActionResponseComponent.h"

#include "PlayerGameState.h"

#include "Game/World/Player/PlayerActionSystem.h"

class PlayerLocal : public LambdaEngine::System
{
public:
	DECL_UNIQUE_CLASS(PlayerLocal);
	PlayerLocal();
	virtual ~PlayerLocal();

	void Init();

	void TickMainThread(LambdaEngine::Timestamp deltaTime);
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

	void TickLocalPlayerAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);
	void DoAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final {};

	void SendGameState(const PlayerGameState& gameState);
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
	bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	void Reconcile();
	void ReplayGameStatesBasedOnServerGameState(PlayerGameState* pGameStates, uint32 count, const PacketPlayerActionResponseComponent::Packet& gameStateServer);
	bool CompareGameStates(const PlayerGameState& gameStateLocal, const PacketPlayerActionResponseComponent::Packet& gameStateServer);

private:
	LambdaEngine::IDVector m_Entities;
	int32 m_NetworkUID;
	int32 m_SimulationTick;
	LambdaEngine::IClient* m_pClient;
	LambdaEngine::PlayerActionSystem m_PlayerActionSystem;
	LambdaEngine::TArray<PlayerGameState> m_FramesToReconcile;
};