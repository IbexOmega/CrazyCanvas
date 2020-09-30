#include "Game/ECS/Systems/Networking/ClientSystem.h"
#include "Game/ECS/Systems/Networking/InterpolationSystem.h"
#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Engine/EngineLoop.h"

#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Components/Networking/InterpolationComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#define EPSILON 0.01f

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
		m_Entities(),
		m_NetworkUID(-1),
		m_pInterpolationSystem(nullptr)
	{

	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
		SAFEDELETE(m_pInterpolationSystem);
	}

	void ClientSystem::Init()
	{
		ClientDesc clientDesc = {};
		clientDesc.PoolSize = 1024;
		clientDesc.MaxRetries = 10;
		clientDesc.ResendRTTMultiplier = 5.0F;
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

		SubscribeToPacketType(NetworkSegment::TYPE_ENTITY_CREATE, std::bind(&ClientSystem::OnPacketCreateEntity, this, std::placeholders::_1));
		SubscribeToPacketType(NetworkSegment::TYPE_PLAYER_ACTION, std::bind(&ClientSystem::OnPacketPlayerAction, this, std::placeholders::_1));

		//m_pInterpolationSystem = DBG_NEW InterpolationSystem();
	}

	bool ClientSystem::Connect(IPAddress* pAddress)
	{
		if (!m_pClient->Connect(IPEndPoint(pAddress, 4444)))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::SubscribeToPacketType(uint16 packetType, const std::function<void(NetworkSegment*)>& func)
	{
		m_PacketSubscribers[packetType].PushBack(func);
	}

	Entity ClientSystem::GetEntityFromNetworkUID(int32 networkUID) const
	{
		auto pair = m_Entities.find(networkUID);
		ASSERT(pair != m_Entities.end());
		return pair->second;
	}

	bool ClientSystem::IsLocalClient(int32 networkUID) const
	{
		return m_NetworkUID == networkUID;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		if (m_pClient->IsConnected() && m_Entities.size() > 0)
		{
			int8 deltaForward	= int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
			int8 deltaLeft		= int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
			BinaryEncoder encoder(pPacket);
			encoder.WriteInt32(m_SimulationTick);
			encoder.WriteInt8(deltaForward);
			encoder.WriteInt8(deltaLeft);
			m_pClient->SendReliable(pPacket);

			ECSCore* pECS = ECSCore::GetInstance();
			auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

			if (!pPositionComponents)
				return;

			PositionComponent& positionComponent = pPositionComponents->GetData(GetEntityPlayer());
			GameState gameState = {};

			gameState.SimulationTick	= m_SimulationTick;
			gameState.DeltaForward		= deltaForward;
			gameState.DeltaLeft			= deltaLeft;
			gameState.Position			= positionComponent.Position;

			m_FramesToReconcile.PushBack(gameState);
			m_SimulationTick++;

			Reconcile();
		}
	}

	Entity ClientSystem::GetEntityPlayer() const
	{
		return GetEntityFromNetworkUID(m_NetworkUID);
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		NetworkDebugger::RenderStatistics(m_pClient);
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnConnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		UNREFERENCED_VARIABLE(pClient);

		auto iterator = m_PacketSubscribers.find(pPacket->GetType());
		if (iterator != m_PacketSubscribers.end())
		{
			const TArray<std::function<void(NetworkSegment*)>>& functions = iterator->second;
			for (const auto& func : functions)
			{
				func(pPacket);
			}
		}
		else
		{
			LOG_WARNING("No packet subscription of type: %hu", pPacket->GetType());
		}
	}

	void ClientSystem::OnPacketCreateEntity(NetworkSegment* pPacket)
	{
		BinaryDecoder decoder(pPacket);
		bool isMySelf		= decoder.ReadBool();
		int32 networkUID	= decoder.ReadInt32();
		glm::vec3 position	= decoder.ReadVec3();
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

	void ClientSystem::OnPacketPlayerAction(NetworkSegment* pPacket)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		GameState serverGameState = {};

		BinaryDecoder decoder(pPacket);
		int32 networkUID				= decoder.ReadInt32();
		serverGameState.SimulationTick	= decoder.ReadInt32();
		serverGameState.Position		= decoder.ReadVec3();

		if (IsLocalClient(networkUID))
		{
			m_FramesProcessedByServer.PushBack(serverGameState);
		}
		else
		{
			auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

			PositionComponent& positionComponent = pPositionComponents->GetData(GetEntityFromNetworkUID(networkUID));
			
			positionComponent.Position	= serverGameState.Position;
			positionComponent.Dirty		= true;
		}
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
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

		if (IsLocalClient(networkUID))
		{
			pECS->AddComponent<ControllableComponent>(entity,	{ true });
		}
		/*else
			pECS->AddComponent<InterpolationComponent>(entity, { glm::vec3(0.0f), glm::vec3(0.0f), 0 });*/
	}

	void ClientSystem::Reconcile()
	{
		while (!m_FramesProcessedByServer.IsEmpty())
		{
			ASSERT(m_FramesProcessedByServer[0].SimulationTick == m_FramesToReconcile[0].SimulationTick);

			if (!CompareGameStates(m_FramesProcessedByServer[0], m_FramesToReconcile[0]))
			{
				ReplayGameStatesBasedOnServerGameState(m_FramesToReconcile.GetData(), m_FramesToReconcile.GetSize(), m_FramesProcessedByServer[0]);
			}

			m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
			m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.Begin());
		}
	}

	void ClientSystem::ReplayGameStatesBasedOnServerGameState(const GameState* gameStates, uint32 count, const GameState& gameStateServer)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity entityPlayer = GetEntityPlayer();

		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		PositionComponent& positionComponent = pPositionComponents->GetData(entityPlayer);

		positionComponent.Position	= gameStateServer.Position;
		positionComponent.Dirty		= true;

		//Replay all game states since the game state which resulted in prediction ERROR
		for (uint32 i = 0; i < count; i++)
		{
			PlayerUpdate(entityPlayer, gameStates[i]);
		}
	}

	bool ClientSystem::CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer)
	{
		if (glm::distance(gameStateLocal.Position, gameStateServer.Position) > EPSILON)
		{
			return false;
		}

		return true;
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