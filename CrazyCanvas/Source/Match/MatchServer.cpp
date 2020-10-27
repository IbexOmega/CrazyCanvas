#include "Match/MatchServer.h"

#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

#include "Math/Random.h"

#include "Networking/API/ClientRemoteBase.h"

#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Rendering/ImGuiRenderer.h"

#include "Multiplayer/Packet/CreateLevelObject.h"

#include <imgui.h>

#define RENDER_MATCH_INFORMATION

MatchServer::~MatchServer()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::UnregisterEventHandler<OnFlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
}

bool MatchServer::InitInternal()
{
	using namespace LambdaEngine;
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::RegisterEventHandler<OnFlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);

	return true;
}

void MatchServer::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		uint32 numFlags = 0;
		m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, numFlags);

		if (numFlags == 0)
			SpawnFlag();
	}

	//Render Some Server Match Information

#if defined(RENDER_MATCH_INFORMATION)
	ImGuiRenderer::Get().DrawUI([this]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		if (ImGui::Begin("Match Panel"))
		{
			if (m_pLevel != nullptr)
			{
				uint32 numFlags = 0;
				Entity* pFlags = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, numFlags);

				if (numFlags > 0)
				{
					Entity flagEntity = pFlags[0];

					const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
					ImGui::Text("Flag Status: %s", flagParentComponent.Attached ? "Carried" : "Not Carried");

					if (flagParentComponent.Attached)
					{
						if (ImGui::Button("Drop Flag"))
						{
							FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, glm::vec3(0.0f, 2.0f, 0.0f));
						}
					}
				}
				else
				{
					ImGui::Text("Flag Status: Not Spawned");
				}
			}
		}

		ImGui::End();
	});
#endif
}

void MatchServer::SpawnFlag()
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	uint32 numFlagSpawnPoints = 0;
	Entity* pFlagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN, numFlagSpawnPoints);

	if (numFlagSpawnPoints > 0)
	{
		const FlagSpawnComponent& flagSpawnComponent	= pECS->GetConstComponent<FlagSpawnComponent>(pFlagSpawnPointEntities[0]);
		const PositionComponent& positionComponent		= pECS->GetConstComponent<PositionComponent>(pFlagSpawnPointEntities[0]);

		float r		= Random::Float32(0.0f, flagSpawnComponent.Radius);
		float theta = Random::Float32(0.0f, glm::two_pi<float32>());

		CreateFlagDesc createDesc = {};
		createDesc.Position		= positionComponent.Position + r * glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta));
		createDesc.Scale		= glm::vec3(1.0f);
		createDesc.Rotation		= glm::identity<glm::quat>();

		TArray<Entity> createdFlagEntities;
		if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createDesc, createdFlagEntities))
		{
			VALIDATE(createdFlagEntities.GetSize() == 1);

			//Tell the bois that we created a flag
			const ClientMap& clients = ServerSystem::GetInstance().GetServer()->GetClients();

			CreateLevelObject packet;
			packet.LevelObjectType			= ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;
			packet.Position					= createDesc.Position;
			packet.Forward					= GetForward(createDesc.Rotation);
			packet.Flag.ParentNetworkUID	= INT32_MAX;

			//Tell the bois that we created a flag
			for (Entity entity : createdFlagEntities)
			{
				packet.NetworkUID = entity;
				ServerSystem::GetInstance().GetServer()->SendReliableStructBroadcast(packet, PacketType::CREATE_LEVEL_OBJECT);
			}
		}
		else
		{
			LOG_ERROR("[MatchServer]: Failed to create Flag");
		}
	}
}

void MatchServer::SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	uint32 numPlayerSpawnPoints = 0;
	Entity* pPlayerSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN, numPlayerSpawnPoints);

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	glm::vec3 position(0.0f, 5.0f, 0.0f);
	glm::vec3 forward(0.0f, 0.0f, 1.0f);

	for (uint32 i = 0; i < numPlayerSpawnPoints; i++)
	{
		Entity spawnPoint = pPlayerSpawnPointEntities[i];

		const TeamComponent& teamComponent = pTeamComponents->GetConstData(spawnPoint);

		if (teamComponent.TeamIndex == m_NextTeamIndex)
		{
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(spawnPoint);
			position = positionComponent.Position + glm::vec3(0.0f, 1.0f, 0.0f);
			forward = glm::normalize(-glm::vec3(position.x, 0.0f, position.z));
			break;
		}
	}

	CreatePlayerDesc createPlayerDesc =
	{
		.pClient		= pClient,
		.Position		= position,
		.Forward		= forward,
		.Scale			= glm::vec3(1.0f),
		.TeamIndex		= m_NextTeamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		CreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position			= position;
		packet.Forward			= forward;
		packet.Player.TeamIndex	= m_NextTeamIndex;

		for (Entity playerEntity : createdPlayerEntities)
		{
			packet.Player.IsMySelf	= true;
			packet.NetworkUID		= playerEntity;

			pClient->SendReliableStruct(packet, PacketType::CREATE_LEVEL_OBJECT, nullptr);

			packet.Player.IsMySelf	= false;
			pClient->SendReliableStructBroadcast(packet, PacketType::CREATE_LEVEL_OBJECT, nullptr, true);
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
	}

	m_NextTeamIndex = (m_NextTeamIndex + 1) % 2;
}

bool MatchServer::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

bool MatchServer::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	IClient* pClient = event.pClient;

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
	ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();

	//Send currently existing players to the new client
	{
		uint32 otherPlayerCount = 0;
		Entity* pOtherPlayerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, otherPlayerCount);

		CreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Player.IsMySelf	= false;

		for (uint32 i = 0; i < otherPlayerCount; i++)
		{
			Entity otherPlayerEntity = pOtherPlayerEntities[i];
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(otherPlayerEntity);
			const RotationComponent& rotationComponent = pRotationComponents->GetConstData(otherPlayerEntity);
			const TeamComponent& teamComponent = pTeamComponents->GetConstData(otherPlayerEntity);

			packet.NetworkUID		= otherPlayerEntity;
			packet.Position			= positionComponent.Position;
			packet.Forward			= GetForward(rotationComponent.Quaternion);
			packet.Player.TeamIndex	= teamComponent.TeamIndex;
			pClient->SendReliableStruct(packet, PacketType::CREATE_LEVEL_OBJECT);
		}
	}

	//Create a player for the new client, also sends the new player to the connected clients
	{
		SpawnPlayer((ClientRemoteBase*)pClient);
	}

	//Send flag data to clients
	{
		uint32 flagCount = 0;
		Entity* pFlagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, flagCount);

		CreateLevelObject packet;
		packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;

		for (uint32 i = 0; i < flagCount; i++)
		{
			Entity flagEntity = pFlagEntities[i];
			const PositionComponent& positionComponent	= pPositionComponents->GetConstData(flagEntity);
			const RotationComponent& rotationComponent	= pRotationComponents->GetConstData(flagEntity);
			const ParentComponent& parentComponent		= pParentComponents->GetConstData(flagEntity);

			packet.NetworkUID				= flagEntity;
			packet.Position					= positionComponent.Position;
			packet.Forward					= GetForward(rotationComponent.Quaternion);
			packet.Flag.ParentNetworkUID	= parentComponent.Parent;
			pClient->SendReliableStruct(packet, PacketType::CREATE_LEVEL_OBJECT);
		}
	}

	return true;
}

bool MatchServer::OnFlagDelivered(const OnFlagDeliveredEvent& event)
{
	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		uint32 numFlags = 0;
		Entity* pFlags = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, numFlags);

		if (numFlags > 0)
		{
			Entity flagEntity = pFlags[0];
			FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, glm::vec3(0.0f, 2.0f, 0.0f));
		}
	}

	uint32 newScore = GetScore(event.TeamIndex) + 1;
	SetScore(event.TeamIndex, newScore);

	const ClientMap& clients = ServerSystem::GetInstance().GetServer()->GetClients();

	if (!clients.empty())
	{
		ClientRemoteBase* pClient = clients.begin()->second;

		NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::TEAM_SCORED);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt32(event.TeamIndex);
		encoder.WriteUInt32(newScore);

		pClient->SendReliableBroadcast(pPacket);
	}

	return true;
}
