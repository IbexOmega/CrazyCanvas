#include "Match/MatchServer.h"
#include "Match/Match.h"

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

	Job job;
	job.Components =
	{
		{ ComponentPermissions::RW,	FlagSpawnComponent::Type() },
		{ ComponentPermissions::RW,	PositionComponent::Type() },
	};

	job.Function = [this]()
	{
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

				for (Entity entity : createdFlagEntities)
				{
					for (auto& clientPair : clients)
					{
						//Send to everyone already connected
						NetworkSegment* pPacket = clientPair.second->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
						BinaryEncoder encoder(pPacket);
						encoder.WriteUInt8(uint8(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG));
						encoder.WriteInt32((int32)entity);
						encoder.WriteInt32(INT32_MAX);
						encoder.WriteVec3(createDesc.Position);
						encoder.WriteQuat(createDesc.Rotation);
						clientPair.second->SendReliable(pPacket, nullptr);
					}
				}
			}
			else
			{
				LOG_ERROR("[MatchServer]: Failed to create Flag");
			}
		}
	};

	pECS->ScheduleJobASAP(job);
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

	uint32 otherPlayerCount = 0;
	Entity* pOtherPlayerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, otherPlayerCount);

	for (uint32 i = 0; i < otherPlayerCount; i++)
	{
		Entity otherPlayerEntity = pOtherPlayerEntities[i];
		const PositionComponent& positionComponent = pPositionComponents->GetConstData(otherPlayerEntity);
		const RotationComponent& rotationComponent = pRotationComponents->GetConstData(otherPlayerEntity);
		const TeamComponent& teamComponent = pTeamComponents->GetConstData(otherPlayerEntity);

		NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt8(uint8(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER));
		encoder.WriteBool(false);
		encoder.WriteInt32((int32)otherPlayerEntity);
		encoder.WriteVec3(positionComponent.Position);
		encoder.WriteVec3(GetForward(rotationComponent.Quaternion));
		encoder.WriteUInt32(teamComponent.TeamIndex);
		pClient->SendReliable(pPacket, nullptr);
	}

	static glm::vec3 position(2.0f, 10.0f, 0.0f);
	static glm::vec3 forward(1.0f, 0.0f, 0.0f);
	static uint32 teamIndex = 0;

	CreatePlayerDesc createPlayerDesc =
	{
		.pClient		= pClient,
		.Position		= position,
		.Forward		= forward,
		.Scale			= glm::vec3(1.0f),
		.TeamIndex		= teamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		const ClientMap& clients = reinterpret_cast<ClientRemoteBase*>(pClient)->GetClients();

		for (Entity playerEntity : createdPlayerEntities)
		{
			{
				NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
				BinaryEncoder encoder = BinaryEncoder(pPacket);
				encoder.WriteUInt8(uint8(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER));
				encoder.WriteBool(true);
				encoder.WriteInt32((int32)playerEntity);
				encoder.WriteVec3(position);
				encoder.WriteVec3(forward);
				encoder.WriteUInt32(teamIndex);

				//Todo: 2nd argument should not be nullptr if we want a little info
				pClient->SendReliable(pPacket, nullptr);
			}

			for (auto& clientPair : clients)
			{
				if (clientPair.second != pClient)
				{
					//Send to everyone already connected
					NetworkSegment* pPacket = clientPair.second->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
					BinaryEncoder encoder(pPacket);
					encoder.WriteUInt8(uint8(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER));
					encoder.WriteBool(false);
					encoder.WriteInt32((int32)playerEntity);
					encoder.WriteVec3(position);
					encoder.WriteVec3(forward);
					encoder.WriteUInt32(teamIndex);
					clientPair.second->SendReliable(pPacket, nullptr);
				}
			}
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
		return false;
	}

	teamIndex = (teamIndex + 1) % 2;

	uint32 flagCount = 0;
	Entity* pFlagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, flagCount);

	for (uint32 i = 0; i < flagCount; i++)
	{
		Entity flagEntity = pFlagEntities[i];
		const PositionComponent& positionComponent	= pPositionComponents->GetConstData(flagEntity);
		const RotationComponent& rotationComponent	= pRotationComponents->GetConstData(flagEntity);
		const ParentComponent& parentComponent		= pParentComponents->GetConstData(flagEntity);

		NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt8(uint8(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG));
		encoder.WriteInt32((int32)flagEntity);
		encoder.WriteInt32(parentComponent.Parent);
		encoder.WriteVec3(positionComponent.Position);
		encoder.WriteQuat(rotationComponent.Quaternion);
		pClient->SendReliable(pPacket, nullptr);
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

	if (newScore == m_MatchDesc.MaxScore && !clients.empty())
	{
		ClientRemoteBase* pClient = clients.begin()->second;

		NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::GAME_OVER);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt32(event.TeamIndex);	// winning team

		pClient->SendReliableBroadcast(pPacket);

		Match::ResetMatch();
	}


	return true;
}
