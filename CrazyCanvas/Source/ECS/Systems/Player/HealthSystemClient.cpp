#include "ECS/Systems/Player/HealthSystemClient.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"
#include "Game/ECS/Components/Team/TeamComponent.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include "Teams/TeamHelper.h"
#include "MeshPaint/MeshPaintHandler.h"

/*
* HealthSystemClient
*/

HealthSystemClient::~HealthSystemClient()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &HealthSystemClient::OnPlayerAliveUpdated);
}

void HealthSystemClient::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>*						pHealthComponents			= pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<PacketHealthChanged>>*	pHealthChangedComponents	= pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	for (Entity entity : m_HealthEntities)
	{
		HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
		PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);
		for (const PacketHealthChanged& packet : packets.GetPacketsReceived())
		{
			if (healthComponent.CurrentHealth != packet.CurrentHealth)
			{
				healthComponent.CurrentHealth = packet.CurrentHealth;

				// Is this the local player
				bool isLocal = false;
				for (Entity playerEntity : m_LocalPlayerEntities)
				{
					if (playerEntity == entity)
					{
						isLocal = true;
						break;
					}
				}

				// Send event to notify systems that a player got hit
				const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(entity);
				const glm::vec3 position = positionComp.Position;

				PlayerHitEvent event(entity, position, isLocal);
				EventQueue::SendEvent(event);
			}
		}
	}
}

bool HealthSystemClient::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= NDA;
		playerGroup.Rotation.Permissions	= NDA;
		playerGroup.Velocity.Permissions	= NDA;

		SystemRegistration systemReg = {};
		HealthSystem::CreateBaseSystemRegistration(systemReg);

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
		{
			.pSubscriber = &m_LocalPlayerEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() },
			},
			.ComponentGroups = { &playerGroup }
		});

		systemReg.SubscriberRegistration.AdditionalAccesses =
		{
			{ R, TeamComponent::Type() }
		};

		RegisterSystem(TYPE_NAME(HealthSystemClient), systemReg);
	}

	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &HealthSystemClient::OnPlayerAliveUpdated);
	return true;
}

bool HealthSystemClient::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	using namespace LambdaEngine;

	const Entity playerEntity = event.pPlayer->GetEntity();
	LOG_INFO("PlayerAliveUpdatedEvent isDead=%s entity=%u",
		event.pPlayer->IsDead() ? "true" : "false",
		playerEntity);

	if (event.pPlayer->IsDead())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<ChildComponent>* pChildComponents = pECS->GetComponentArray<ChildComponent>();

		const PositionComponent positionComponent	= pECS->GetComponent<PositionComponent>(playerEntity);
		const TeamComponent teamComponent			= pECS->GetComponent<TeamComponent>(playerEntity);

		Entity entity = pECS->CreateEntity();

		pECS->AddComponent<PositionComponent>(entity, positionComponent);
		pECS->AddComponent<RotationComponent>(entity, { true, GetRotationQuaternion(glm::normalize(glm::vec3(0, -1, 0))) });

		pECS->AddComponent<ParticleEmitterComponent>(entity,
			ParticleEmitterComponent{
				.Active = true,
				.OneTime = true,
				.Explosive = 1.f,
				.SpawnDelay = 0.05f,
				.ParticleCount = 512,
				.EmitterShape = EEmitterShape::CONE,
				.Angle = 45.f,
				.VelocityRandomness = 0.5f,
				.Velocity = 7.0,
				.Acceleration = 0.0,
				.Gravity = -9.f,
				.LifeTime = 1.2f,
				.RadiusRandomness = 0.5f,
				.BeginRadius = 0.2f,
				.FrictionFactor = 0.f,
				.RandomStartIndex = true,
				.AnimationCount = 1,
				.FirstAnimationIndex = 6,
				.Color = glm::vec4(TeamHelper::GetTeamColor(teamComponent.TeamIndex), 1.f)
			}
		);

		MeshPaintHandler::AddHitPoint(
			positionComponent.Position,
			glm::vec3(0.0f, -1.0f, 0.0f),
			EPaintMode::PAINT,
			ERemoteMode::CLIENT,
			ETeam(teamComponent.TeamIndex == 1),
			0);

		// Reset child components
		if (pChildComponents->HasComponent(playerEntity))
		{
			const ChildComponent& childComponent = pChildComponents->GetConstData(playerEntity);
			const TArray<std::string>& tags = childComponent.GetTags();
			for (const std::string& tag : tags)
			{
				Entity childEntity = childComponent.GetEntityWithTag(tag);
				MeshPaintHandler::ResetServer(childEntity);
			}
		}

		// Reset player
		MeshPaintHandler::ResetServer(event.pPlayer->GetEntity());
	}

	return true;
}
