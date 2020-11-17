#include "Match/MatchServer.h"
#include "Match/Match.h"

#include "ECS/ECSCore.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Player/HealthSystem.h"

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
#include "Multiplayer/Packet/PacketMatchReady.h"
#include "Multiplayer/Packet/PacketMatchStart.h"
#include "Multiplayer/Packet/PacketMatchBegin.h"

#include "Lobby/PlayerManagerServer.h"

#include <imgui.h>

#define RENDER_MATCH_INFORMATION

MatchServer::~MatchServer()
{
	using namespace LambdaEngine;
	
	EventQueue::UnregisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::UnregisterEventHandler<FlagRespawnEvent>(this, &MatchServer::OnFlagRespawn);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);
}

bool MatchServer::InitInternal()
{
	using namespace LambdaEngine;

	EventQueue::RegisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::RegisterEventHandler<FlagRespawnEvent>(this, &MatchServer::OnFlagRespawn);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);

	return true;
}

void MatchServer::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (m_ShouldBeginMatch && !m_HasBegun)
	{
		m_MatchBeginTimer -= float32(deltaTime.AsSeconds());

		if (m_MatchBeginTimer < 0.0f)
		{
			MatchBegin();
		}
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
				const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
				const ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
				const ComponentArray<PositionComponent>* pPositionComponent = pECS->GetComponentArray<PositionComponent>();

				// Flags
				TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
				for (uint32 f = 0; f < flagEntities.GetSize(); f++)
				{
					Entity flagEntity = flagEntities[f];

					std::string name = "Flag " + std::to_string(f) + ": [EntityID=" + std::to_string(flagEntity) + "]";
					if (ImGui::TreeNode(name.c_str()))
					{
						const TeamComponent& flagTeamComponent			= pTeamComponents->GetConstData(flagEntity);
						const ParentComponent& flagParentComponent		= pParentComponents->GetConstData(flagEntity);
						const PositionComponent& flagPositionComponent	= pPositionComponent->GetConstData(flagEntity);
						ImGui::Text("Flag Team Index: %u", flagTeamComponent.TeamIndex);
						ImGui::Text("Flag Position: [ %f, %f, %f ]", flagPositionComponent.Position.x, flagPositionComponent.Position.y, flagPositionComponent.Position.z);
						ImGui::Text("Flag Status: %s", flagParentComponent.Attached ? "Carried" : "Not Carried");

						if (flagParentComponent.Attached)
						{
							if (ImGui::Button("Drop Flag"))
							{
								glm::vec3 flagPosition;
								if (CreateFlagSpawnProperties(flagTeamComponent.TeamIndex, flagPosition))
								{
									FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, flagPosition);
								}
							}
						}

						ImGui::TreePop();
					}
				}

				// Player
				TArray<Entity> playerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);
				if (!playerEntities.IsEmpty())
				{
					ComponentArray<ChildComponent>*		pChildComponents	= pECS->GetComponentArray<ChildComponent>();
					ComponentArray<HealthComponent>*	pHealthComponents	= pECS->GetComponentArray<HealthComponent>();
					ComponentArray<WeaponComponent>*	pWeaponComponents	= pECS->GetComponentArray<WeaponComponent>();

					ImGui::Text("Player Status:");
					for (Entity playerEntity : playerEntities)
					{
						const Player* pPlayer = PlayerManagerServer::GetPlayer(playerEntity);
						if (!pPlayer)
							continue;

						std::string name = pPlayer->GetName() + ": [EntityID=" + std::to_string(playerEntity) + "]";
						if (ImGui::TreeNode(name.c_str()))
						{
							const HealthComponent& health = pHealthComponents->GetConstData(playerEntity);
							ImGui::Text("Health: %u", health.CurrentHealth);

							const ChildComponent& children = pChildComponents->GetConstData(playerEntity);
							Entity weapon = children.GetEntityWithTag("weapon");

							const WeaponComponent& weaponComp = pWeaponComponents->GetConstData(weapon);

							auto waterAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
							if(waterAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Water Ammunition: %u/%u", waterAmmo->second.first, waterAmmo->second.second);

							auto paintAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
							if (paintAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Paint Ammunition: %u/%u", paintAmmo->second.first, paintAmmo->second.second);


							if (ImGui::Button("Kill"))
							{
								MatchServer::KillPlayer(playerEntity, UINT32_MAX);
							}
							
							ImGui::SameLine();

							if (ImGui::Button("Disconnect"))
							{
								ServerHelper::DisconnectPlayer(pPlayer, "Kicked");
							}

							ImGui::TreePop();
						}
					}
				}
				else
				{
					ImGui::Text("Player Status: No players");
				}
			}
		}

		ImGui::End();
	});
#endif
}

void MatchServer::BeginLoading()
{
	using namespace LambdaEngine;
	LOG_INFO("SERVER: Loading started!");

	const THashTable<uint64, Player>& players = PlayerManagerBase::GetPlayers();

	for (auto& pair : players)
	{
		SpawnPlayer(pair.second);
	}

	if (m_pLevel != nullptr)
	{
		if (m_GameModeHasFlag)
		{
			if (m_MatchDesc.GameMode == EGameMode::CTF_COMMON_FLAG)
			{
				SpawnFlag(UINT8_MAX); //UINT8_MAX means no Team
			}
			else
			{
				for (uint32 t = 0; t < m_MatchDesc.NumTeams; t++)
				{
					SpawnFlag(t);
				}
			}
		}
	}

	PacketMatchReady packet;
	ServerHelper::SendBroadcast(packet);
}

void MatchServer::MatchStart()
{
	m_HasBegun = false;
	m_ShouldBeginMatch = true;
	m_MatchBeginTimer = MATCH_BEGIN_COUNTDOWN_TIME;

	PacketMatchStart matchStartPacket;
	ServerHelper::SendBroadcast(matchStartPacket);

	LOG_INFO("SERVER: Match Start");
}

void MatchServer::MatchBegin()
{
	m_HasBegun = true;
	m_ShouldBeginMatch = false;

	PacketMatchBegin matchBeginPacket;
	ServerHelper::SendBroadcast(matchBeginPacket);

	LOG_INFO("SERVER: Match Begin");
}

void MatchServer::SpawnPlayer(const Player& player)
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
		.ClientUID	= player.GetUID(),
		.Position	= position,
		.Forward	= forward,
		.Scale		= glm::vec3(1.0f),
		.TeamIndex	= m_NextTeamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position			= position;
		packet.Forward			= forward;
		packet.Player.TeamIndex = m_NextTeamIndex;

		ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();
		for (Entity playerEntity : createdPlayerEntities)
		{
			const ChildComponent& childComp = pCreatedChildComponents->GetConstData(playerEntity);
			packet.Player.ClientUID			= player.GetUID();
			packet.NetworkUID				= playerEntity;
			packet.Player.WeaponNetworkUID	= childComp.GetEntityWithTag("weapon");
			ServerHelper::SendBroadcast(packet);
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
	}

	m_NextTeamIndex = (m_NextTeamIndex + 1) % 2;
}

void MatchServer::FixedTickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(deltaTime);

	{
		std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
		for (Entity playerEntity : m_PlayersToKill)
		{
			LOG_INFO("SERVER: Player=%u DIED", playerEntity);
			DoKillPlayer(playerEntity);
		}

		m_PlayersToKill.Clear();
	}
}

void MatchServer::SpawnFlag(uint8 teamIndex)
{
	using namespace LambdaEngine;

	glm::vec3 flagPosition;

	if (CreateFlagSpawnProperties(teamIndex, flagPosition))
	{
		CreateFlagDesc createDesc = {};
		createDesc.Position		= flagPosition;
		createDesc.Scale		= glm::vec3(1.0f);
		createDesc.Rotation		= glm::identity<glm::quat>();
		createDesc.TeamIndex	= teamIndex;

		TArray<Entity> createdFlagEntities;
		if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createDesc, createdFlagEntities))
		{
			VALIDATE(createdFlagEntities.GetSize() == 1);

			PacketCreateLevelObject packet;
			packet.LevelObjectType			= ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;
			packet.Position					= createDesc.Position;
			packet.Forward					= GetForward(createDesc.Rotation);
			packet.Flag.ParentNetworkUID	= INT32_MAX;
			packet.Flag.TeamIndex			= teamIndex;

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
	else
	{
		LOG_ERROR("[MatchServer]: Failed to find suitable place to spawn flag");
	}
}

bool MatchServer::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity	= event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;
	createProjectileDesc.MeshComponent	= event.MeshComponent;
	createProjectileDesc.Angle			= event.Angle;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchServer]: Failed to create projectile!");
	}

	LOG_INFO("SERVER: Weapon fired");
	return false;
}

void MatchServer::DeleteGameLevelObject(LambdaEngine::Entity entity)
{
	m_pLevel->DeleteObject(entity);

	PacketDeleteLevelObject packet;
	packet.NetworkUID = entity;

	ServerHelper::SendBroadcast(packet);
}

bool MatchServer::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	VALIDATE(event.pClient != nullptr);

	const Player* pPlayer = PlayerManagerBase::GetPlayer(event.pClient);
	if (pPlayer)
	{
		MatchServer::KillPlayer(pPlayer->GetEntity(), UINT32_MAX);
	}
	

	// TODO: Fix this
	//DeleteGameLevelObject(playerEntity);

	return true;
}

bool MatchServer::OnFlagDelivered(const FlagDeliveredEvent& event)
{
	using namespace LambdaEngine;

	glm::vec3 flagPosition;

	if (CreateFlagSpawnProperties(event.FlagTeamIndex, flagPosition))
	{
		FlagSystemBase::GetInstance()->OnFlagDropped(event.Entity, flagPosition);

		uint32 newScore = GetScore(event.ScoringTeamIndex) + 1;
		SetScore(event.ScoringTeamIndex, newScore);

		PacketTeamScored packet;
		packet.TeamIndex	= event.ScoringTeamIndex;
		packet.Score		= newScore;
		ServerHelper::SendBroadcast(packet);

		if (newScore == m_MatchDesc.MaxScore) // game over
		{
			PacketGameOver gameOverPacket;
			gameOverPacket.WinningTeamIndex = event.ScoringTeamIndex;

			ServerHelper::SendBroadcast(gameOverPacket);

			ResetMatch();
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to find suitable place to respawn flag after delivery");
	}

	return true;
}

bool MatchServer::OnFlagRespawn(const FlagRespawnEvent& event)
{
	using namespace LambdaEngine;

	glm::vec3 flagPosition;

	if (CreateFlagSpawnProperties(event.FlagTeamIndex, flagPosition))
	{
		FlagSystemBase::GetInstance()->OnFlagDropped(event.Entity, flagPosition);
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to find suitable place to respawn flag");
	}

	return true;
}

void MatchServer::DoKillPlayer(LambdaEngine::Entity playerEntity)
{
	using namespace LambdaEngine;

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<NetworkPositionComponent>* pNetworkPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	if (pNetworkPosComponents != nullptr && pNetworkPosComponents->HasComponent(playerEntity))
	{
		NetworkPositionComponent& positionComp = pECS->GetComponent<NetworkPositionComponent>(playerEntity);

		// Get spawnpoint from level
		const glm::vec3 oldPosition = positionComp.Position;
		glm::vec3 newPosition = glm::vec3(0.0f);

		VALIDATE(m_pLevel != nullptr);

		// Retrive spawnpoints
		TArray<Entity> spawnPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

		ComponentArray<PositionComponent>*	pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<TeamComponent>*		pTeamComponents		= pECS->GetComponentArray<TeamComponent>();

		uint8 playerTeam = pTeamComponents->GetConstData(playerEntity).TeamIndex;
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

		positionComp.Position = newPosition;

		const Player* pPlayer = PlayerManagerServer::GetPlayer(playerEntity);
		LOG_INFO("SERVER: Moving player[%s] to [%.4f, %.4f, %.4f]", 
			pPlayer->GetName().c_str(), 
			newPosition.x,
			newPosition.y,
			newPosition.z);

		// Drop flag if player carries it
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
		for (Entity flagEntity : flagEntities)
		{
			const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
			if (flagParentComponent.Attached && flagParentComponent.Parent == playerEntity)
			{
				FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, oldPosition);
			}
		}

		//Set player alive again
		PlayerManagerServer::SetPlayerAlive(pPlayer, true, nullptr);
	}
	else
	{
		LOG_WARNING("Killed player called for entity(=%u) that does not have a NetworkPositionComponent", playerEntity);
	}
}

void MatchServer::InternalKillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity)
{
	using namespace LambdaEngine;

	const Player* pPlayer		= PlayerManagerServer::GetPlayer(entityToKill);
	const Player* pPlayerKiller = PlayerManagerServer::GetPlayer(killedByEntity);
	PlayerManagerServer::SetPlayerAlive(pPlayer, false, pPlayerKiller);

	std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
	m_PlayersToKill.EmplaceBack(entityToKill);
}

void MatchServer::KillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity)
{
	using namespace LambdaEngine;

	MatchServer* pMatchServer = static_cast<MatchServer*>(Match::GetInstance());
	pMatchServer->InternalKillPlayer(entityToKill, killedByEntity);
}

bool MatchServer::CreateFlagSpawnProperties(uint8 teamIndex, glm::vec3& position)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

	const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	for (Entity flagSpawnPointEntity : flagSpawnPointEntities)
	{
		TeamComponent flagSpawnTeamComponent = {};
		bool flagSpawnHasTeamComponent = pTeamComponents->GetConstIf(flagSpawnPointEntity, flagSpawnTeamComponent);

		bool validSpawnPoint = false;
		if (teamIndex == UINT8_MAX)
		{
			//If the team index is UINT8_MAX the flag is common and we don't care about flag spawn point team
			validSpawnPoint = !flagSpawnHasTeamComponent;
		}
		else
		{
			//Check that the spawn point has the same team index as the flag we want to spawn
			validSpawnPoint = (flagSpawnTeamComponent.TeamIndex == teamIndex);
		}

		if (validSpawnPoint)
		{
			const PositionComponent& flagSpawnPositionComponent		= pECS->GetConstComponent<PositionComponent>(flagSpawnPointEntity);
			position = flagSpawnPositionComponent.Position;
			return true;
		}
	}

	return false;
}
