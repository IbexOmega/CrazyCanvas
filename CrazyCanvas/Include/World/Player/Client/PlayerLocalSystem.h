#pragma once
#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "World/Player/PlayerGameState.h"

#include "Multiplayer/Packet/PacketPlayerActionResponse.h"

class PlayerLocalSystem : public ReplaySystem<PlayerGameState, PacketPlayerActionResponse>
{
public:
	DECL_UNIQUE_CLASS(PlayerLocalSystem);
	
	PlayerLocalSystem();
	virtual ~PlayerLocalSystem() = default;

	void Init() override;

	void TickMainThread(LambdaEngine::Timestamp deltaTime);

	void TickLocalPlayerAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);
	void DoAction(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);

protected:
	virtual void PlaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, PlayerGameState& clientState) override;
	virtual void ReplayGameState(LambdaEngine::Timestamp deltaTime, float32 dt, PlayerGameState& clientState) override;
	virtual void SurrenderGameState(const PacketPlayerActionResponse& serverState) override;
	virtual bool CompareGamesStates(const PlayerGameState& clientState, const PacketPlayerActionResponse& serverState) override;

private:
	void SendGameState(const PlayerGameState& gameState, LambdaEngine::Entity entityPlayer);

private:
	LambdaEngine::IDVector m_Entities;
	PlayerActionSystem m_PlayerActionSystem;
};