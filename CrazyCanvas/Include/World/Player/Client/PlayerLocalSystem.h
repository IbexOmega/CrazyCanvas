#pragma once
#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "World/Player/PlayerGameState.h"

#include "Multiplayer/Packet/PacketPlayerActionResponse.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

class PlayerLocalSystem : public ReplaySystem<PlayerGameState, PacketPlayerActionResponse>
{
public:
	DECL_UNIQUE_CLASS(PlayerLocalSystem);
	
	PlayerLocalSystem();
	virtual ~PlayerLocalSystem() = default;

	void Init() override;

	void TickMainThread(LambdaEngine::Timestamp deltaTime);

	void TickLocalPlayerAction(float32 dt, LambdaEngine::Entity entityPlayer, PlayerGameState* pGameState);
	void DoAction(
		float32 dt, 
		LambdaEngine::VelocityComponent& velocityComponent, 
		LambdaEngine::CharacterColliderComponent& characterColliderComponent, 
		const LambdaEngine::RotationComponent& rotationComponent, 
		PlayerGameState* pGameState);

protected:
	virtual void PlaySimulationTick(float32 dt, PlayerGameState& clientState) override;
	virtual void ReplayGameState(float32 dt, PlayerGameState& clientState) override;
	virtual void SurrenderGameState(const PacketPlayerActionResponse& serverState) override;
	virtual bool CompareGamesStates(const PlayerGameState& clientState, const PacketPlayerActionResponse& serverState) override;

private:
	void SendGameState(const PlayerGameState& gameState, LambdaEngine::Entity entityPlayer);

private:
	LambdaEngine::IDVector m_Entities;
	PlayerActionSystem m_PlayerActionSystem;
};