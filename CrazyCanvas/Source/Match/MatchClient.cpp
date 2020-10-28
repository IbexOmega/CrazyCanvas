#include "Match/MatchClient.h"

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

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

using namespace LambdaEngine;

MatchClient::~MatchClient()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<CreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
}

bool MatchClient::InitInternal()
{
	EventQueue::RegisterEventHandler<PacketReceivedEvent<CreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
	return true;
}

void MatchClient::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

bool MatchClient::OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<CreateLevelObject>& event)
{
	const CreateLevelObject& packet = event.Packet;
	IClient* pClient = event.pClient;

	switch (packet.LevelObjectType)
	{
		case ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER:
		{
			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

			const CameraDesc cameraDesc =
			{
				.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV"),
				.Width = (float)window->GetWidth(),
				.Height = (float)window->GetHeight(),
				.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane"),
				.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane")
			};

			//Todo: Move this ffs
			TArray<GUID_Lambda> animations;
			const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx", animations);
			bool animationsExist = !animations.IsEmpty();

			AnimationComponent robotAnimationComp = {};
			robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
			if (animationsExist)
			{
				robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("dancing", animations[0]));
			}

			CreatePlayerDesc createPlayerDesc =
			{
				.IsLocal			= packet.Player.IsMySelf,
				.PlayerNetworkUID	= packet.NetworkUID,
				.WeaponNetworkUID	= packet.Player.WeaponNetworkUID,
				.pClient			= event.pClient,
				.Position			= packet.Position,
				.Forward			= packet.Forward,
				.Scale				= glm::vec3(1.0f),
				.TeamIndex			= packet.Player.TeamIndex,
				.pCameraDesc		= &cameraDesc,
				.MeshGUID			= robotGUID,
				.AnimationComponent = robotAnimationComp,
			};

			TArray<Entity> createdPlayerEntities;
			if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
			{
				LOG_ERROR("[MatchClient]: Failed to create Player!");
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

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchClient]: Failed to create projectile!");
	}

	LOG_INFO("CLIENT: Weapon fired");
	return true;
}

bool MatchClient::OnPlayerDied(const PlayerDiedEvent& event)
{
	LOG_INFO("CLIENT: Player=%u DIED", event.KilledEntity);
	return false;
}
