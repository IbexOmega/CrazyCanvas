#pragma once
#include "ECS/System.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "World/Player/PlayerGameState.h"

class PlayerLocalSystem : public LambdaEngine::System
{
public:
	DECL_UNIQUE_CLASS(PlayerLocalSystem);
	PlayerLocalSystem();
	virtual ~PlayerLocalSystem();

	void Init();

	void TickMainThread(LambdaEngine::Timestamp deltaTime);
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

	void TickLocalPlayerAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);
	void DoAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };

	void SendGameState(const PlayerGameState& gameState, LambdaEngine::Entity entityPlayer);
	void Reconcile(LambdaEngine::Entity entityPlayer);
	void ReplayGameStatesBasedOnServerGameState(LambdaEngine::Entity entityPlayer, PlayerGameState* pGameStates, uint32 count, const PlayerActionResponse& gameStateServer);
	bool CompareGameStates(const PlayerGameState& gameStateLocal, const PlayerActionResponse& gameStateServer);

private:
	LambdaEngine::IDVector m_Entities;
	int32 m_NetworkUID;
	int32 m_SimulationTick;
	PlayerActionSystem m_PlayerActionSystem;
	LambdaEngine::TArray<PlayerGameState> m_FramesToReconcile;
};