#include "Engine/EngineLoop.h"

#include "Game/ECS/Systems/Networking/Client/ClientSystem.h"

#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/ClientUtilsImpl.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem() :
		m_pClient(nullptr),
		m_CharacterControllerSystem(),
		m_NetworkPositionSystem(),
		m_PlayerSystem()
	{

	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
		MultiplayerUtils::Release();
	}

	void ClientSystem::Init()
	{
		MultiplayerUtils::Init(false);

		ClientDesc clientDesc			= {};
		clientDesc.PoolSize				= 1024;
		clientDesc.MaxRetries			= 10;
		clientDesc.ResendRTTMultiplier	= 5.0F;
		clientDesc.Handler				= this;
		clientDesc.Protocol				= EProtocol::UDP;
		clientDesc.PingInterval			= Timestamp::Seconds(1);
		clientDesc.PingTimeout			= Timestamp::Seconds(10);
		clientDesc.UsePingSystem		= true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);


		MultiplayerUtils::SubscribeToPacketType(NetworkSegment::TYPE_ENTITY_CREATE, std::bind(&ClientSystem::OnPacketCreateEntity, this, std::placeholders::_1, std::placeholders::_2));

		m_CharacterControllerSystem.Init();
		m_NetworkPositionSystem.Init();
		m_PlayerSystem.Init();
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

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		if (m_pClient->IsConnected())
		{
			m_PlayerSystem.FixedTickMainThread(deltaTime, m_pClient);
		}

		m_CharacterControllerSystem.FixedTickMainThread(deltaTime);
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
		MultiplayerUtils::s_pMultiplayerUtility->FirePacketEvent(pClient, pPacket);
	}

	void ClientSystem::OnPacketCreateEntity(IClient* pClient, NetworkSegment* pPacket)
	{
		BinaryDecoder decoder(pPacket);
		bool isMySelf		= decoder.ReadBool();
		int32 networkUID	= decoder.ReadInt32();
		glm::vec3 position	= decoder.ReadVec3();
		glm::vec3 color		= decoder.ReadVec3();

		if (isMySelf)
			m_PlayerSystem.m_NetworkUID = networkUID;

		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		LOG_INFO("Creating Entity with ID %d and NetworkID %d", entity, networkUID);

		MaterialProperties materialProperties = {};
		materialProperties.Roughness = 0.1f;
		materialProperties.Metallic = 0.0f;
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		//TArray<GUID_Lambda> animations;

		//const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		//const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MeshComponent meshComponent;
		meshComponent.MeshGUID = ResourceManager::LoadMeshFromFile("Sphere.obj");
		meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Mirror Material" + std::to_string(entity),
			//robotAlbedoGUID,
			//robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		//AnimationComponent animationComp;
		//animationComp.AnimationGUID = animations[0];


		pECS->AddComponent<PlayerComponent>(entity, PlayerComponent{ .IsLocal = isMySelf });
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.0f) });
		pECS->AddComponent<VelocityComponent>(entity, { true, glm::vec3(0.0f) });
		//pECS->AddComponent<AnimationComponent>(entity,		animationComp);
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<NetworkComponent>(entity, { networkUID });
		pECS->AddComponent<NetworkPositionComponent>(entity, { position, position, EngineLoop::GetTimeSinceStart(), EngineLoop::GetFixedTimestep() });

		const CharacterColliderInfo colliderInfo = {
			.Entity = entity,
			.Position = pECS->GetComponent<PositionComponent>(entity),
			.Rotation = pECS->GetComponent<RotationComponent>(entity),
			.CollisionGroup = FCollisionGroup::COLLISION_GROUP_PLAYER,
			.CollisionMask = FCollisionGroup::COLLISION_GROUP_STATIC | FCollisionGroup::COLLISION_GROUP_PLAYER
		};

		constexpr const float capsuleHeight = 1.8f;
		constexpr const float capsuleRadius = 0.2f;
		CharacterColliderComponent characterColliderComponent;
		PhysicsSystem::GetInstance()->CreateCharacterCapsule(colliderInfo, std::max(0.0f, capsuleHeight - 2.0f * capsuleRadius), capsuleRadius, characterColliderComponent);
		pECS->AddComponent<CharacterColliderComponent>(entity, characterColliderComponent);


		ClientUtilsImpl* pUtil = (ClientUtilsImpl*)MultiplayerUtils::s_pMultiplayerUtility;
		pUtil->RegisterEntity(entity, networkUID);
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
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