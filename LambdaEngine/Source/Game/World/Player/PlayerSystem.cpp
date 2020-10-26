#include "Game/World/Player/PlayerSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/CharacterControllerHelper.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

#include "Game/Multiplayer/GameState.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/IClient.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

#define EPSILON 0.01f

namespace LambdaEngine
{
	PlayerSystem::PlayerSystem() :
		m_EntityOtherStates(),
		m_PlayerActionSystem(),
		m_FramesToReconcile(),
		m_FramesProcessedByServer(),
		m_SimulationTick(0),
		m_NetworkUID(-1)
	{

	}

	PlayerSystem::~PlayerSystem()
	{

	}

	void PlayerSystem::Init()
	{
		EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PlayerSystem::OnPacketReceived);
	}

	void PlayerSystem::TickMainThread(Timestamp deltaTime, IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);

		if (m_NetworkUID >= 0)
		{
			Entity entityPlayer = MultiplayerUtils::GetEntity(m_NetworkUID);
			m_PlayerActionSystem.TickMainThread(deltaTime, entityPlayer);
		}
	}

	void PlayerSystem::FixedTickMainThread(Timestamp deltaTime, IClient* pClient)
	{
		if(m_NetworkUID >= 0)
		{
			Reconcile();

			GameState gameState = {};
			gameState.SimulationTick = m_SimulationTick++;

			Entity entityPlayer = MultiplayerUtils::GetEntity(m_NetworkUID);
			TickLocalPlayerAction(deltaTime, entityPlayer, &gameState);

			TickOtherPlayersAction(deltaTime);

			if (!MultiplayerUtils::IsSingleplayer())
				SendGameState(gameState, pClient);
		}
	}

	void PlayerSystem::SendGameState(const GameState& gameState, IClient* pClient)
	{
		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
		BinaryEncoder encoder(pPacket);
		encoder.WriteInt32(gameState.SimulationTick);
		encoder.WriteQuat(gameState.Rotation);
		encoder.WriteInt8(gameState.DeltaForward);
		encoder.WriteInt8(gameState.DeltaLeft);
		pClient->SendReliable(pPacket);
	}

	void PlayerSystem::TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents	= pECS->GetComponentArray<CharacterColliderComponent>();
		const ComponentArray<NetworkPositionComponent>* pNetPosComponents			= pECS->GetComponentArray<NetworkPositionComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents						= pECS->GetComponentArray<VelocityComponent>();
		const ComponentArray<PositionComponent>* pPositionComponents				= pECS->GetComponentArray<PositionComponent>();

		const NetworkPositionComponent& netPosComponent	= pNetPosComponents->GetConstData(entityPlayer);
		const VelocityComponent& velocityComponent		= pVelocityComponents->GetConstData(entityPlayer);
		const PositionComponent& positionComponent		= pPositionComponents->GetConstData(entityPlayer);

		NetworkPositionComponent& mutableNetPosComponent = const_cast<NetworkPositionComponent&>(netPosComponent);
		mutableNetPosComponent.PositionLast		= positionComponent.Position; //Lerpt from the current interpolated position (The rendered one)
		mutableNetPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();

		m_PlayerActionSystem.DoAction(deltaTime, entityPlayer, pGameState);

		CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

		pGameState->Position = netPosComponent.Position;
		pGameState->Velocity = velocityComponent.Velocity;

		m_FramesToReconcile.PushBack(*pGameState);
	}

	void PlayerSystem::TickOtherPlayersAction(Timestamp deltaTime)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		const ComponentArray<NetworkPositionComponent>* pNetPosComponents	= pECS->GetComponentArray<NetworkPositionComponent>();
		ComponentArray<CharacterColliderComponent>* pCharacterColliders		= pECS->GetComponentArray<CharacterColliderComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents				= pECS->GetComponentArray<VelocityComponent>();
		const ComponentArray<PositionComponent>* pPositionComponents		= pECS->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>* pRotationComponents				= pECS->GetComponentArray<RotationComponent>();

		for (auto& pair : m_EntityOtherStates)
		{
			Entity entity = pair.first;

			const NetworkPositionComponent& constNetPosComponent = pNetPosComponents->GetConstData(entity);
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(entity);
			VelocityComponent& velocityComponent = pVelocityComponents->GetData(entity);

			TArray<GameState>& gameStates = pair.second;

			if (!gameStates.IsEmpty())
			{
				for (const GameState& gameState : gameStates)
				{
					const RotationComponent& constRotationComponent = pRotationComponents->GetConstData(entity);

					if (constRotationComponent.Quaternion != gameState.Rotation)
					{
						RotationComponent& rotationComponent	= const_cast<RotationComponent&>(constRotationComponent);
						rotationComponent.Quaternion			= gameState.Rotation;
						rotationComponent.Dirty					= true;
					}

					if (glm::any(glm::notEqual(constNetPosComponent.Position, gameState.Position)))
					{
						NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
						netPosComponent.PositionLast	= positionComponent.Position;
						netPosComponent.Position		= gameState.Position;
						netPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();
						netPosComponent.Dirty			= true;
					}

					velocityComponent.Velocity = gameState.Velocity;

					CharacterControllerHelper::TickForeignCharacterController(dt, entity, pCharacterColliders, pNetPosComponents, pVelocityComponents);
				}

				gameStates.Clear();
			}
			else //Data does not exist for the current frame :(
			{
				velocityComponent.Velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

				NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
				netPosComponent.PositionLast		= positionComponent.Position;
				netPosComponent.Position			+= velocityComponent.Velocity * dt;
				netPosComponent.TimestampStart		= EngineLoop::GetTimeSinceStart();
				netPosComponent.Dirty				= true;

				CharacterControllerHelper::TickForeignCharacterController(dt, entity, pCharacterColliders, pNetPosComponents, pVelocityComponents);
			}
		}
	}

	bool PlayerSystem::OnPacketReceived(const PacketReceivedEvent& event)
	{
		if (event.Type == NetworkSegment::TYPE_PLAYER_ACTION)
		{
			GameState serverGameState = {};

			BinaryDecoder decoder(event.pPacket);
			int32 networkUID = decoder.ReadInt32();

			decoder.ReadInt32(serverGameState.SimulationTick);
			decoder.ReadVec3(serverGameState.Position);
			decoder.ReadVec3(serverGameState.Velocity);
			decoder.ReadQuat(serverGameState.Rotation);

			if (networkUID == m_NetworkUID)
			{
				m_FramesProcessedByServer.PushBack(serverGameState);
			}
			else
			{
				Entity entity = MultiplayerUtils::GetEntity(networkUID);
				m_EntityOtherStates[entity].PushBack(serverGameState);
			}
			return true;
		}
		return false;
	}

	void PlayerSystem::Reconcile()
	{
		while (!m_FramesProcessedByServer.IsEmpty())
		{
			ASSERT(m_FramesProcessedByServer[0].SimulationTick == m_FramesToReconcile[0].SimulationTick);

			if (!CompareGameStates(m_FramesToReconcile[0], m_FramesProcessedByServer[0]))
			{
				ReplayGameStatesBasedOnServerGameState(m_FramesToReconcile.GetData(), m_FramesToReconcile.GetSize(), m_FramesProcessedByServer[0]);
			}

			m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
			m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.Begin());
		}
	}

	void PlayerSystem::ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity entityPlayer = MultiplayerUtils::GetEntity(m_NetworkUID);

		ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
		ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

		netPosComponent.Position = gameStateServer.Position;
		velocityComponent.Velocity = gameStateServer.Velocity;

		//Replay all game states since the game state which resulted in prediction ERROR

		// TODO: Rollback other entities not just the player

		const Timestamp deltaTime = EngineLoop::GetFixedTimestep();
		const float32 dt = (float32)deltaTime.AsSeconds();

		for (uint32 i = 1; i < count; i++)
		{
			GameState& gameState = pGameStates[i];

			/*
			* Returns the velocity based on key presses
			*/
			PlayerActionSystem::ComputeVelocity(gameState.Rotation, gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);

			/*
			* Sets the position of the PxController taken from the PositionComponent.
			* Move the PxController using the VelocityComponent by the deltatime.
			* Calculates a new Velocity based on the difference of the last position and the new one.
			* Sets the new position of the PositionComponent
			*/
			CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

			gameState.Position = netPosComponent.Position;
			gameState.Velocity = velocityComponent.Velocity;
		}
	}

	bool PlayerSystem::CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer)
	{
		bool result = true;
		if (glm::distance(gameStateLocal.Position, gameStateServer.Position) > EPSILON)
		{
			LOG_ERROR("Prediction Error, Tick: %d, Position: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Position.x, gameStateLocal.Position.y, gameStateLocal.Position.z, gameStateServer.Position.x, gameStateServer.Position.y, gameStateServer.Position.z);
			result = false;
		}

		if (glm::distance(gameStateLocal.Velocity, gameStateServer.Velocity) > EPSILON)
		{
			LOG_ERROR("Prediction Error, Tick: %d, Velocity: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Velocity.x, gameStateLocal.Velocity.y, gameStateLocal.Velocity.z, gameStateServer.Velocity.x, gameStateServer.Velocity.y, gameStateServer.Velocity.z);
			result = false;
		}

		return result;
	}
}