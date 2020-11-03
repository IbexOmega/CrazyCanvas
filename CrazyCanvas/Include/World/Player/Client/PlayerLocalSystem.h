#pragma once
#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "World/Player/PlayerGameState.h"

#include "Multiplayer/Packet/PacketPlayerActionResponse.h"

class PlayerLocalSystem : public ReplayBaseSystem
{
public:
	DECL_UNIQUE_CLASS(PlayerLocalSystem);
	
	PlayerLocalSystem();
	virtual ~PlayerLocalSystem() = default;

	void Init() override;

	void TickMainThread(LambdaEngine::Timestamp deltaTime);
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override;

	void TickLocalPlayerAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);
	void DoAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);

protected:
	virtual void PlaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, int32 simulationTick) override;
	virtual void ReplaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, uint32 i, int32 simulationTick) override;
	virtual void SurrenderGameState(int32 simulationTick) override;
	virtual int32 GetNextAvailableSimulationTick() override;
	virtual bool CompareNextGamesStates(int32 simulationTick) override;
	virtual void DeleteGameState(int32 simulationTick) override;

private:
	void SendGameState(const PlayerGameState& gameState, LambdaEngine::Entity entityPlayer);
	//void Reconcile(LambdaEngine::Entity entityPlayer);
	//void ReplayGameStatesBasedOnServerGameState(LambdaEngine::Entity entityPlayer, PlayerGameState* pGameStates, uint32 count, const PacketPlayerActionResponse& gameStateServer);
	bool CompareGameStates(const PlayerGameState& gameStateLocal, const PacketPlayerActionResponse& gameStateServer);

private:
	LambdaEngine::IDVector m_Entities;
	PlayerActionSystem m_PlayerActionSystem;
	LambdaEngine::TArray<PlayerGameState> m_FramesToReconcile;

	LambdaEngine::TArray<PacketPlayerActionResponse> m_FramesProcessedByServer;
};