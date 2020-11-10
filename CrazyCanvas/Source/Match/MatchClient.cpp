#include "Match/MatchClient.h"
#include "Match/Match.h"

#include "Multiplayer/Packet/PacketType.h"

#include "World/LevelObjectCreator.h"
#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/CommonApplication.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Lobby/PlayerManagerClient.h"

#include "Events/MatchEvents.h"

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

#include "Resources/ResourceManager.h"

#include "Audio/AudioAPI.h"

using namespace LambdaEngine;

MatchClient::~MatchClient()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketCreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketDeleteLevelObject>>(this, &MatchClient::OnPacketDeleteLevelObjectReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketMatchReady>>(this, &MatchClient::OnPacketMatchReadyReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketMatchStart>>(this, &MatchClient::OnPacketMatchStartReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketMatchBegin>>(this, &MatchClient::OnPacketMatchBeginReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameOver>>(this, &MatchClient::OnPacketGameOverReceived);
}

bool MatchClient::InitInternal()
{
	if (MultiplayerUtils::IsSingleplayer())
	{
		m_HasBegun = false;
		m_ClientSideBegun = false;
		m_MatchBeginTimer = MATCH_BEGIN_COUNTDOWN_TIME;
	}

	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketCreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketDeleteLevelObject>>(this, &MatchClient::OnPacketDeleteLevelObjectReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketMatchReady>>(this, &MatchClient::OnPacketMatchReadyReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketMatchStart>>(this, &MatchClient::OnPacketMatchStartReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketMatchBegin>>(this, &MatchClient::OnPacketMatchBeginReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameOver>>(this, &MatchClient::OnPacketGameOverReceived);

	m_CountdownSoundEffects[4] = ResourceManager::LoadSoundEffect2DFromFile("Countdown/five.wav");
	m_CountdownSoundEffects[3] = ResourceManager::LoadSoundEffect2DFromFile("Countdown/four.wav");
	m_CountdownSoundEffects[2] = ResourceManager::LoadSoundEffect2DFromFile("Countdown/three.wav");
	m_CountdownSoundEffects[1] = ResourceManager::LoadSoundEffect2DFromFile("Countdown/two.wav");
	m_CountdownSoundEffects[0] = ResourceManager::LoadSoundEffect2DFromFile("Countdown/one.wav");
	m_CountdownDoneSoundEffect = ResourceManager::LoadSoundEffect2DFromFile("Countdown/go.mp3");

	return true;
}

void MatchClient::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	if (!m_ClientSideBegun)
	{
		float32 previousTimer = m_MatchBeginTimer;
		m_MatchBeginTimer -= float32(deltaTime.AsSeconds());

		if (previousTimer >= 5.0f && m_MatchBeginTimer < 5.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownSoundEffects[4])->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(5);
		}
		else if (previousTimer >= 4.0f && m_MatchBeginTimer < 4.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownSoundEffects[3])->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(4);
		}
		else if (previousTimer >= 3.0f && m_MatchBeginTimer < 3.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownSoundEffects[2])->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(3);
		}
		else if (previousTimer >= 2.0f && m_MatchBeginTimer < 2.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownSoundEffects[1])->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(2);
		}
		else if (previousTimer >= 1.0f && m_MatchBeginTimer < 1.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownSoundEffects[0])->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(1);
		}
		else if (previousTimer >= 0.0f && m_MatchBeginTimer < 0.0f)
		{
			ResourceManager::GetSoundEffect2D(m_CountdownDoneSoundEffect)->PlayOnce(0.1f);
			EventQueue::SendEvent<MatchCountdownEvent>(0);

			m_ClientSideBegun = true;
			m_CountdownHideTimer = 1.0f;

			if (MultiplayerUtils::IsSingleplayer())
			{
				m_HasBegun = true;
			}
		}	
	}

	if (m_CountdownHideTimer >= 0.0f)
	{
		m_CountdownHideTimer -= float32(deltaTime.AsSeconds());

		if (m_CountdownHideTimer < 0.0f)
		{
			EventQueue::SendEvent<MatchCountdownEvent>(UINT8_MAX);
		}
	}
}

bool MatchClient::OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<PacketCreateLevelObject>& event)
{
	const PacketCreateLevelObject& packet = event.Packet;

	switch (packet.LevelObjectType)
	{
		case ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER:
		{
			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

			const CameraDesc cameraDesc =
			{
				.FOVDegrees = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV),
				.Width = (float)window->GetWidth(),
				.Height = (float)window->GetHeight(),
				.NearPlane = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_NEAR_PLANE),
				.FarPlane = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FAR_PLANE)
			};

			CreatePlayerDesc createPlayerDesc =
			{
				.ClientUID			= event.pClient->GetUID(),
				.IsLocal			= packet.Player.ClientUID == event.pClient->GetUID(),
				.PlayerNetworkUID	= packet.NetworkUID,
				.WeaponNetworkUID	= packet.Player.WeaponNetworkUID,
				.Position			= packet.Position,
				.Forward			= packet.Forward,
				.Scale				= glm::vec3(1.0f),
				.TeamIndex			= packet.Player.TeamIndex,
				.pCameraDesc		= &cameraDesc,
			};

			TArray<Entity> createdPlayerEntities;
			if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
			{
				LOG_ERROR("[MatchClient]: Failed to create Player!");
			}

			// Notify systems that a new player connected (Not myself tho)
			if (!createPlayerDesc.IsLocal)
			{
				PlayerConnectedEvent connectedEvent(createdPlayerEntities[0], packet.Position);
				EventQueue::SendEvent(connectedEvent);
			}

			break;
		}
		case ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG:
		{
			Entity parentEntity = MultiplayerUtils::GetEntity(packet.Flag.ParentNetworkUID);

			CreateFlagDesc createFlagDesc =
			{
				.NetworkUID = packet.NetworkUID,
				.ParentEntity = parentEntity,
				.Position = packet.Position,
				.Scale = glm::vec3(1.0f),
				.Rotation = glm::quatLookAt(packet.Forward, g_DefaultUp),
			};

			TArray<Entity> createdFlagEntities;
			if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createFlagDesc, createdFlagEntities))
			{
				LOG_ERROR("[MatchClient]: Failed to create Flag!");
			}

			break;
		}
	}
	return true;
}

bool MatchClient::OnPacketTeamScoredReceived(const PacketReceivedEvent<PacketTeamScored>& event)
{
	const PacketTeamScored& packet = event.Packet;
	SetScore(packet.TeamIndex, packet.Score);
	return true;
}

bool MatchClient::OnPacketDeleteLevelObjectReceived(const PacketReceivedEvent<PacketDeleteLevelObject>& event)
{
	const PacketDeleteLevelObject& packet = event.Packet;
	Entity entity = MultiplayerUtils::GetEntity(packet.NetworkUID);

	if(entity != UINT32_MAX)
		m_pLevel->DeleteObject(entity);

	return true;
}

bool MatchClient::OnPacketMatchStartReceived(const PacketReceivedEvent<PacketMatchStart>& event)
{
	UNREFERENCED_VARIABLE(event);

	m_HasBegun = false;
	m_ClientSideBegun = false;
	m_MatchBeginTimer = MATCH_BEGIN_COUNTDOWN_TIME;

	LOG_INFO("CLIENT: Match Start");

	return true;
}

bool MatchClient::OnPacketMatchReadyReceived(const PacketReceivedEvent<PacketMatchReady>& event)
{
	//Makes sure we have finished loading everything....

	PlayerManagerClient::SetLocalPlayerStateLoaded();

	return true;
}

bool MatchClient::OnPacketMatchBeginReceived(const PacketReceivedEvent<PacketMatchBegin>& event)
{
	UNREFERENCED_VARIABLE(event);

	m_HasBegun = true;

	LOG_INFO("CLIENT: Match Begin");

	return true;
}

bool MatchClient::OnPacketGameOverReceived(const PacketReceivedEvent<PacketGameOver>& event)
{
	const PacketGameOver& packet = event.Packet;

	LOG_INFO("Game Over, Winning team is %d", packet.WinningTeamIndex);
	ResetMatch();

	return true;
}

bool MatchClient::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FireDirection	= event.Direction;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity = event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.MeshComponent	= event.MeshComponent;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchClient]: Failed to create projectile!");
	}

	LOG_INFO("CLIENT: Weapon fired");
	return true;
}