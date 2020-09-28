#include "Game/ECS/Systems/Networking/ClientSystem.h"
#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Engine/EngineLoop.h"

#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#define EPSILON 0.00001f

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem() :
		m_ControllableEntities(),
		m_pClient(nullptr),
		m_FramesToReconcile(),
		m_FramesProcessedByServer(),
		m_SimulationTick(0),
		m_LastNetworkSimulationTick(0),
		m_Entities()
	{
		ClientDesc clientDesc = {};
		clientDesc.PoolSize = 1024;
		clientDesc.MaxRetries = 10;
		clientDesc.ResendRTTMultiplier = 3.0F;
		clientDesc.Handler = this;
		clientDesc.Protocol = EProtocol::UDP;
		clientDesc.PingInterval = Timestamp::Seconds(1);
		clientDesc.PingTimeout = Timestamp::Seconds(3);
		clientDesc.UsePingSystem = true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);


		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{{{R, ControllableComponent::Type()}, {R, PositionComponent::Type()} }, {}, &m_ControllableEntities}
		};
		systemReg.Phase = 0;

		RegisterSystem(systemReg);
	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
	}

	bool ClientSystem::Connect(IPAddress* address)
	{
		if (!m_pClient->Connect(IPEndPoint(address, 4444)))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		if (m_pClient->IsConnected())
		{
			int8 deltaForward = int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
			int8 deltaLeft = int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
			BinaryEncoder encoder(pPacket);
			encoder.WriteInt32(m_SimulationTick);
			encoder.WriteInt8(deltaForward);
			encoder.WriteInt8(deltaLeft);
			m_pClient->SendUnreliable(pPacket);

			ECSCore* pECS = ECSCore::GetInstance();
			auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

			if (!pPositionComponents)
				return;

			auto pair = m_Entities.find(m_NetworkUID);

			ASSERT(pair != m_Entities.end());

			PositionComponent& positionComponent = pPositionComponents->GetData(pair->second);
			GameState gameState = {};

			gameState.SimulationTick	= m_SimulationTick;
			gameState.DeltaForward		= deltaForward;
			gameState.DeltaLeft			= deltaLeft;

			if (deltaForward != 0)
			{
				gameState.Position.x = positionComponent.Position.x + 1.0f * EngineLoop::GetFixedTimestep().AsSeconds() * deltaForward;
			}

			if (deltaLeft != 0)
			{
				gameState.Position.z = positionComponent.Position.z + 1.0f * EngineLoop::GetFixedTimestep().AsSeconds() * deltaLeft;
			}

			{
				std::scoped_lock<SpinLock> lock(m_Lock);
				m_FramesToReconcile.PushBack(gameState);
			}
			m_SimulationTick++;

			if (!m_FramesProcessedByServer.IsEmpty())
			{
				Reconcile();
			}
		}
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		NetworkDebugger::RenderStatistics(m_pClient);
	}

	void ClientSystem::Tick(Timestamp deltaTime)
	{
		
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{

	}

	void ClientSystem::OnConnected(IClient* pClient)
	{

	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{

	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{

	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_ENTITY_CREATE)
		{
			BinaryDecoder decoder(pPacket);
			bool isMySelf = decoder.ReadBool();
			int32 networkUID = decoder.ReadInt32();
			glm::vec3 position = decoder.ReadVec3();
			glm::vec3 color		= decoder.ReadVec3();

			if (isMySelf)
				m_NetworkUID = networkUID;

			Job addEntityJob;
			addEntityJob.Components =
			{
				{ RW, PositionComponent::Type()		},
				{ RW, RotationComponent::Type()		},
				{ RW, ScaleComponent::Type()		},
				{ RW, MeshComponent::Type()			},
				{ RW, NetworkComponent::Type()		},
				{ RW, ControllableComponent::Type() }
			};
			addEntityJob.Function = std::bind(&ClientSystem::CreateEntity, this, networkUID, position, color);

			ECSCore::GetInstance()->ScheduleJobASAP(addEntityJob);
		}
		else if (pPacket->GetType() == NetworkSegment::TYPE_PLAYER_ACTION)
		{
			ECSCore* pECS = ECSCore::GetInstance();

			GameState serverGameState = {};

			BinaryDecoder decoder(pPacket);
			int32 networkUID					= decoder.ReadInt32();
			serverGameState.SimulationTick		= decoder.ReadInt32();//added
			serverGameState.Position			= decoder.ReadVec3();

			if (networkUID == m_NetworkUID)
			{
				std::scoped_lock<SpinLock> lock(m_Lock);
				m_FramesProcessedByServer.PushBack(serverGameState);
			}
			else
			{
				auto pair = m_Entities.find(networkUID);

				if (pair != m_Entities.end())
				{
					auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

					PositionComponent& positionComponent = pPositionComponents->GetData(pair->second);

					positionComponent.Position	= serverGameState.Position;
					positionComponent.Dirty		= true;
				}
				else
				{
					LOG_ERROR("NetworkUID: %d is not registred on Client", networkUID);
				}
			}



		}
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{

	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{

	}

	void ClientSystem::CreateEntity(int32 networkUID, const glm::vec3& position, const glm::vec3& color)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		LOG_INFO("Creating Entity with ID %d and NetworkID %d", entity, networkUID);

		MaterialProperties materialProperties = {};
		materialProperties.Roughness = 0.1f;
		materialProperties.Metallic = 0.0f;
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		MeshComponent meshComponent;
		meshComponent.MeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Mirror Material" + std::to_string(entity),
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		pECS->AddComponent<PositionComponent>(entity,		{ position, true });
		pECS->AddComponent<RotationComponent>(entity,		{ glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity,			{ glm::vec3(1.0f), true });
		pECS->AddComponent<MeshComponent>(entity,			meshComponent);
		pECS->AddComponent<NetworkComponent>(entity,		{ networkUID });

		m_Entities.insert({ networkUID, entity });

		if (m_NetworkUID == networkUID)
		{
			pECS->AddComponent<ControllableComponent>(entity,	{ true });
		}
	}

	void ClientSystem::Reconcile()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		if (!pPositionComponents)
			return;

		GameState ServerState = {};

		//want to start reading from incoming networkSimulationTick

		auto pair = m_Entities.find(m_NetworkUID);

		ASSERT(pair != m_Entities.end());

		std::scoped_lock<SpinLock> lock(m_Lock);
		while (!m_FramesProcessedByServer.IsEmpty())
		{
			ASSERT(m_FramesProcessedByServer[0].SimulationTick == m_FramesToReconcile[0].SimulationTick);

			if (glm::distance(m_FramesProcessedByServer[0].Position, m_FramesToReconcile[0].Position) > EPSILON) //checks for position prediction ERROR
			{
				PositionComponent& positionComponent = pPositionComponents->GetData(pair->second);

				positionComponent.Position	= m_FramesProcessedByServer[0].Position;
				positionComponent.Dirty		= true;

				//Replay all game states since the game state which resulted in prediction ERROR
				for (int32 i = 1; i < m_FramesToReconcile.GetSize(); i++)
				{
					PlayerUpdate(m_FramesToReconcile[i]); 
				}
			}

			m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
			m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.Begin());
		}
	}

	void ClientSystem::PlayerUpdate(const GameState& gameState)
	{
		auto pair = m_Entities.find(m_NetworkUID);

		ASSERT(pair != m_Entities.end());
		PlayerMovementSystem::GetInstance().Move(pair->second, EngineLoop::GetFixedTimestep(), gameState.DeltaForward, gameState.DeltaLeft);
	}


	void ClientSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
	}

	void ClientSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ClientSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}