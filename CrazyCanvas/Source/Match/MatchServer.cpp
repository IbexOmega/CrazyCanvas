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
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

#include "Math/Random.h"

#include "Networking/API/ClientRemoteBase.h"

#include "Rendering/ImGuiRenderer.h"

#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketGameOver.h"

#include <imgui.h>

#define RENDER_MATCH_INFORMATION

MatchServer::~MatchServer()
{
	using namespace LambdaEngine;
	
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::UnregisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::UnregisterEventHandler<PlayerDiedEvent>(this, &MatchServer::OnPlayerDied);
}

bool MatchServer::InitInternal()
{
	using namespace LambdaEngine;

	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &MatchServer::OnClientConnected);
	EventQueue::RegisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::RegisterEventHandler<PlayerDiedEvent>(this, &MatchServer::OnPlayerDied);

	return true;
}

void MatchServer::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (flagEntities.IsEmpty())
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
				TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

				if (!flagEntities.IsEmpty())
				{
					Entity flagEntity = flagEntities[0];

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

	TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

	if (!flagSpawnPointEntities.IsEmpty())
	{
		Entity flagSpawnPoint = flagSpawnPointEntities[0];
		const FlagSpawnComponent& flagSpawnComponent	= pECS->GetConstComponent<FlagSpawnComponent>(flagSpawnPoint);
		const PositionComponent& positionComponent		= pECS->GetConstComponent<PositionComponent>(flagSpawnPoint);

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

			PacketCreateLevelObject packet;
			packet.LevelObjectType			= ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;
			packet.Position					= createDesc.Position;
			packet.Forward					= GetForward(createDesc.Rotation);
			packet.Flag.ParentNetworkUID	= INT32_MAX;

			//Tell the bois that we created a flag
			for (Entity entity : createdFlagEntities)
			{
				packet.NetworkUID = entity;

				ServerHelper::SendBroadcast(packet);
			}
		}
		else
		{
			LOG_ERROR("[MatchServer]: Failed to create Flag");
		}
	}
}

bool MatchServer::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FireDirection	= event.Direction;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity	= event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;
	createProjectileDesc.MeshComponent	= event.MeshComponent;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchServer]: Failed to create projectile!");
	}

	LOG_INFO("SERVER: Weapon fired");
	return false;
}

bool MatchServer::OnPlayerDied(const PlayerDiedEvent& event)
{
	using namespace LambdaEngine;

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	LOG_INFO("SERVER: Player=%u DIED", event.KilledEntity);

	ECSCore* pECS = ECSCore::GetInstance();
	NetworkPositionComponent& positionComp = pECS->GetComponent<NetworkPositionComponent>(event.KilledEntity);

	// Get spawnpoint from level
	glm::vec3 newPosition = glm::vec3(0.0f);
	if (m_pLevel != nullptr)
	{
		TArray<Entity> spawnPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

		ComponentArray<PositionComponent>*	pPositionComponents	= pECS->GetComponentArray<PositionComponent>();
		ComponentArray<TeamComponent>*		pTeamComponents		= pECS->GetComponentArray<TeamComponent>();

		uint8 playerTeam = pTeamComponents->GetConstData(event.KilledEntity).TeamIndex;
		for (Entity spawnEntity : spawnPoints)
		{
			if (pTeamComponents->HasComponent(spawnEntity))
			{
				if (pTeamComponents->GetConstData(spawnEntity).TeamIndex == playerTeam)
				{
					newPosition = pPositionComponents->GetConstData(spawnEntity).Position;
				}
			}
		}
	}

	positionComp.Position = newPosition;
	return true;
}

void MatchServer::SpawnPlayer(LambdaEngine::ClientRemoteBase* pClient)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> playerSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	glm::vec3 position(0.0f, 5.0f, 0.0f);
	glm::vec3 forward(0.0f, 0.0f, 1.0f);

	for (Entity spawnPoint : playerSpawnPointEntities)
	{
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
		.Position		= position,
		.Forward		= forward,
		.Scale			= glm::vec3(1.0f),
		.TeamIndex		= m_NextTeamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position			= position;
		packet.Forward			= forward;
		packet.Player.TeamIndex	= m_NextTeamIndex;

		ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();
		for (Entity playerEntity : createdPlayerEntities)
		{
			const ChildComponent& childComp = pCreatedChildComponents->GetConstData(playerEntity);
			packet.Player.IsMySelf	= true;
			packet.NetworkUID		= playerEntity;
			packet.Player.WeaponNetworkUID = childComp.GetEntityWithTag("weapon");

			ServerHelper::Send(pClient, packet);

			packet.Player.IsMySelf	= false;
			ServerHelper::SendBroadcast(packet, nullptr, pClient);
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
	}

	m_NextTeamIndex = (m_NextTeamIndex + 1) % 2;
}

void MatchServer::DeleteGameLevelObject(LambdaEngine::Entity entity)
{
	m_pLevel->DeleteObject(entity);

	PacketDeleteLevelObject packet;
	packet.NetworkUID = entity;

	ServerHelper::SendBroadcast(packet);
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
	ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();

	//Send currently existing players to the new client
	{
		TArray<Entity> playerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Player.IsMySelf	= false;

		for (Entity otherPlayerEntity : playerEntities)
		{
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(otherPlayerEntity);
			const RotationComponent& rotationComponent = pRotationComponents->GetConstData(otherPlayerEntity);
			const TeamComponent& teamComponent = pTeamComponents->GetConstData(otherPlayerEntity);
			const ChildComponent& childComp = pCreatedChildComponents->GetConstData(otherPlayerEntity);

			packet.NetworkUID				= otherPlayerEntity;
			packet.Player.WeaponNetworkUID	= childComp.GetEntityWithTag("weapon");
			packet.Position			= positionComponent.Position;
			packet.Forward			= GetForward(rotationComponent.Quaternion);
			packet.Player.TeamIndex	= teamComponent.TeamIndex;
			ServerHelper::Send(pClient, packet);
		}
	}

	//Create a player for the new client, also sends the new player to the connected clients
	{
		SpawnPlayer((ClientRemoteBase*)pClient);
	}

	//Send flag data to clients
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		PacketCreateLevelObject packet;
		packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;

		for (Entity flagEntity : flagEntities)
		{
			const PositionComponent& positionComponent	= pPositionComponents->GetConstData(flagEntity);
			const RotationComponent& rotationComponent	= pRotationComponents->GetConstData(flagEntity);
			const ParentComponent& parentComponent		= pParentComponents->GetConstData(flagEntity);

			packet.NetworkUID				= flagEntity;
			packet.Position					= positionComponent.Position;
			packet.Forward					= GetForward(rotationComponent.Quaternion);
			packet.Flag.ParentNetworkUID	= parentComponent.Parent;
			ServerHelper::Send(pClient, packet);
		}
	}

	return true;
}

bool MatchServer::OnFlagDelivered(const FlagDeliveredEvent& event)
{
	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (!flagEntities.IsEmpty())
		{
			Entity flagEntity = flagEntities[0];
			FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, glm::vec3(0.0f, 2.0f, 0.0f));
		}
	}

	uint32 newScore = GetScore(event.TeamIndex) + 1;
	SetScore(event.TeamIndex, newScore);


	PacketTeamScored packet;
	packet.TeamIndex	= event.TeamIndex;
	packet.Score		= newScore;
	ServerHelper::SendBroadcast(packet);

	if (newScore == m_MatchDesc.MaxScore) // game over
	{
		PacketGameOver gameOverPacket;
		gameOverPacket.WinningTeamIndex = event.TeamIndex;

		ServerHelper::SendBroadcast(gameOverPacket);

		ResetMatch();
	}


	return true;
}
