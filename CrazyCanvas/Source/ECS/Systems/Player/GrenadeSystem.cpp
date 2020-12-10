#include "ECS/Systems/Player/GrenadeSystem.h"

#include "Application/API/Events/EventQueue.h"
#include "ECS/Components/Player/GrenadeComponent.h"
#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Input/API/InputActionSystem.h"
#include "Lobby/PlayerManagerClient.h"
#include "Physics/CollisionGroups.h"
#include "Resources/ResourceManager.h"
#include "World/LevelObjectCreator.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"
#include "Lobby/PlayerManagerBase.h"
#include "Physics/PhysicsEvents.h"
#include "Rendering/EntityMaskManager.h"

GrenadeSystem::~GrenadeSystem()
{
	using namespace LambdaEngine;
	if (!MultiplayerUtils::IsServer())
	{
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &GrenadeSystem::OnKeyPress);
	}

	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGrenadeThrown>>(this, &GrenadeSystem::OnPacketGrenadeThrownReceived);
}

bool GrenadeSystem::Init()
{
	using namespace LambdaEngine;

	if (!MultiplayerUtils::IsServer())
	{
		m_GrenadeMesh = GUID_NONE;
		ResourceManager::LoadMeshFromFile("Grenade.glb", m_GrenadeMesh);
		if (m_GrenadeMesh == GUID_NONE)
		{
			return false;
		}

		EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &GrenadeSystem::OnKeyPress);
	}

	{
		SystemRegistration systemReg;
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_Grenades,
				.ComponentAccesses =
				{
					{ NDA, PositionComponent::Type() },
					{ NDA, GrenadeComponent::Type() },
					{ RW, GrenadeComponent::Type() }
				}
			},
			{
				.pSubscriber = &m_ForeignPlayers,
				.ComponentAccesses =
				{
					{ NDA, PlayerForeignComponent::Type() },
					{ NDA, TeamComponent::Type() }
				}
			}
		};

		systemReg.SubscriberRegistration.AdditionalAccesses =
		{
			{ RW, GrenadeWielderComponent::Type() }
		};

		RegisterSystem(TYPE_NAME(GrenadeSystem), systemReg);
	}

	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGrenadeThrown>>(this, &GrenadeSystem::OnPacketGrenadeThrownReceived);

	return true;
}

void GrenadeSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<GrenadeComponent>* pGrenadeComponents = pECS->GetComponentArray<GrenadeComponent>();

	for (Entity grenade : m_Grenades)
	{
		GrenadeComponent& grenadeComp = pGrenadeComponents->GetData(grenade);
		grenadeComp.ExplosionCountdown -= dt;

		if (grenadeComp.ExplosionCountdown <= 0.0f)
		{
			Explode(grenade);
		}
	}

	ComponentArray<GrenadeWielderComponent>* pGrenadeWielderComponents = pECS->GetComponentArray<GrenadeWielderComponent>();
	if (pGrenadeWielderComponents)
	{
		const TArray<Entity>& grenadeWielderEntities = pGrenadeWielderComponents->GetIDs();
		for (Entity grenadeWielder : grenadeWielderEntities)
		{
			GrenadeWielderComponent& grenadeWielderComp = pGrenadeWielderComponents->GetData(grenadeWielder);
			const uint32 offCooldown = grenadeWielderComp.ThrowCooldown <= 0.0f;
			grenadeWielderComp.ThrowCooldown -= dt * offCooldown;
		}
	}
}

bool GrenadeSystem::OnKeyPress(const LambdaEngine::KeyPressedEvent& keyPressEvent)
{
	using namespace LambdaEngine;

	if (InputActionSystem::IsActionBoundToKey(EAction::ACTION_ATTACK_GRENADE, keyPressEvent.Key))
	{
		const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();

		ECSCore* pECS = ECSCore::GetInstance();

		Entity throwingPlayer = pLocalPlayer->GetEntity();

		PositionComponent playerPosComp;
		RotationComponent playerRotComp;
		if (pECS->GetConstComponentIf(throwingPlayer, playerPosComp) &&
			pECS->GetConstComponentIf(throwingPlayer, playerRotComp))
		{
			const glm::vec3 position = playerPosComp.Position;
			const glm::vec3 velocity = GetForward(playerRotComp.Quaternion) * GRENADE_INITIAL_SPEED;

			if (TryThrowGrenade(throwingPlayer, position, velocity))
			{
				PacketGrenadeThrown packetGrenadeThrown =
				{
					.Position = position,
					.Velocity = velocity,
					.PlayerUID = pLocalPlayer->GetUID()
				};

				ClientHelper::Send(packetGrenadeThrown);
			}
		}
	}

	return false;
}

bool GrenadeSystem::TryThrowGrenade(LambdaEngine::Entity throwingPlayer, const glm::vec3& position, const glm::vec3& velocity)
{
	using namespace LambdaEngine;
	ECSCore* pECS = ECSCore::GetInstance();

	GrenadeWielderComponent grenadeWielderComp;
	if (pECS->GetComponentIf(throwingPlayer, grenadeWielderComp) && grenadeWielderComp.ThrowCooldown <= 0.0f)
	{
		return ThrowGrenade(throwingPlayer, position, velocity);
	}

	return false;
}

bool GrenadeSystem::ThrowGrenade(LambdaEngine::Entity throwingPlayer, const glm::vec3& position, const glm::vec3& velocity)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	VelocityComponent playerVelComp;
	TeamComponent playerTeamComp;
	if (!pECS->GetConstComponentIf(throwingPlayer, playerVelComp) ||
		!pECS->GetConstComponentIf(throwingPlayer, playerTeamComp))
	{
		return false;
	}

	const Entity grenade = pECS->CreateEntity();
	const DynamicCollisionCreateInfo collisionInfo =
	{
		/* Entity */	 		grenade,
		/* Detection Method */	ECollisionDetection::CONTINUOUS,
		/* Position */	 		pECS->AddComponent(grenade, PositionComponent{ .Position = position }),
		/* Scale */				pECS->AddComponent(grenade, ScaleComponent({ .Scale = glm::vec3(0.1f) })),
		/* Rotation */			pECS->AddComponent(grenade, RotationComponent({ .Quaternion = glm::identity<glm::quat>() })),
		{
			{
				/* Shape Type */		EShapeType::SIMULATION,
				/* Geometry Type */		EGeometryType::SPHERE,
				/* Geometry Params */	{ .Radius = 0.15f },
				/* CollisionGroup */	(uint32)FCollisionGroup::COLLISION_GROUP_DYNAMIC |
										(uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PROJECTILE,
				/* CollisionMask */		(uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
										(uint32)FCollisionGroup::COLLISION_GROUP_STATIC,
				/* EntityID*/			throwingPlayer,
			},
		},
		/* Velocity */			pECS->AddComponent(grenade, VelocityComponent({ .Velocity = velocity }))
	};

	const DynamicCollisionComponent projectileCollisionComp = PhysicsSystem::GetInstance()->CreateDynamicActor(collisionInfo);
	pECS->AddComponent<DynamicCollisionComponent>(grenade, projectileCollisionComp);
	pECS->AddComponent<GrenadeComponent>(grenade, GrenadeComponent({ .ExplosionCountdown = GRENADE_FUSE_TIME }));
	pECS->AddComponent<ProjectileComponent>(grenade, ProjectileComponent({ .AmmoType = EAmmoType::AMMO_TYPE_PAINT, .Angle = 0, .Owner = throwingPlayer }));
	pECS->AddComponent<TeamComponent>(grenade, TeamComponent(playerTeamComp));

	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<MeshComponent>(grenade, MeshComponent({ .MeshGUID = m_GrenadeMesh, .MaterialGUID = GUID_MATERIAL_DEFAULT }));
	}

	EntityMaskManager::AddExtensionToEntity(grenade, ProjectileComponent::Type(), nullptr);
	EntityMaskManager::AddExtensionToEntity(grenade, GrenadeComponent::Type(), nullptr);

	return true;
}

void GrenadeSystem::Explode(LambdaEngine::Entity grenade)
{
	using namespace LambdaEngine;

	const Job explodeJob =
	{
		.Components =
		{
			{ R, PositionComponent::Type() },
			{ R, TeamComponent::Type() }
		},
		.Function = [this, grenade]()
		{
			ECSCore* pECS = ECSCore::GetInstance();

			if (MultiplayerUtils::IsServer())
			{
				const glm::vec3& grenadePos = pECS->GetConstComponent<PositionComponent>(grenade).Position;
				const uint8 grenadeTeam = pECS->GetConstComponent<TeamComponent>(grenade).TeamIndex;

				TArray<Entity> players;
				FindPlayersWithinBlast(players, grenadePos, grenadeTeam);

				if (!players.IsEmpty())
				{
					RaycastToPlayers(players, grenade, grenadePos, grenadeTeam);
				}
			}

			pECS->RemoveEntity(grenade);
		}
	};

	ECSCore::GetInstance()->ScheduleJobASAP(explodeJob);
}

void GrenadeSystem::FindPlayersWithinBlast(LambdaEngine::TArray<LambdaEngine::Entity>& players, const glm::vec3& grenadePosition, uint8 grenadeTeam)
{
	using namespace LambdaEngine;

	players.Reserve(m_ForeignPlayers.Size());

	ComponentArray<TeamComponent>* pTeamComponents = ECSCore::GetInstance()->GetComponentArray<TeamComponent>();

	// Use a PhysX overlap query to see which players are within the blast radius
	const OverlapQueryInfo overlapQueryInfo =
	{
		.GeometryType = EGeometryType::SPHERE,
		.GeometryParams =
		{
			.Radius = GRENADE_BLAST_RADIUS
		},
		.Position = grenadePosition,
		.Rotation = glm::identity<glm::quat>()
	};

	const QueryFilterData queryFilterData =
	{
		.IncludedGroup = FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
	};

	constexpr const uint32 maxOverlaps = 10;
	std::array<PxOverlapHit, maxOverlaps> overlapsUserBuffer;
	PxOverlapBuffer overlapsBuffer(overlapsUserBuffer.data(), (PxU32)overlapsUserBuffer.size());

	if (PhysicsSystem::GetInstance()->QueryOverlap(overlapQueryInfo, overlapsBuffer, &queryFilterData))
	{
		const PxOverlapHit* pOverlaps = overlapsBuffer.getTouches();
		const uint32 overlapCount = overlapsBuffer.getNbTouches();

		for (uint32 overlapNr = 0; overlapNr < overlapCount; overlapNr++)
		{
			const PxOverlapHit& overlap = pOverlaps[overlapNr];
			const ActorUserData* pActorUserData = reinterpret_cast<const ActorUserData*>(overlap.actor->userData);
			const Entity hitPlayer = pActorUserData->Entity;

			TeamComponent playerTeamComp;
			PositionComponent playerPosComp;
			if (pTeamComponents->GetConstIf(hitPlayer, playerTeamComp) && playerTeamComp.TeamIndex != grenadeTeam)
			{
				players.PushBack(hitPlayer);
			}
		}
	}
}

void GrenadeSystem::RaycastToPlayers(const LambdaEngine::TArray<LambdaEngine::Entity>& players, LambdaEngine::Entity grenadeEntity, const glm::vec3& grenadePosition, uint8 grenadeTeam)
{
	using namespace LambdaEngine;

	/*	Send 3 rays at different places of the players: At the bottom, the middle and top of the collision body.
		Use the epsilon to nudge the rays away from the collision body's edges, to make sure rays don't miss because of
		precision errors. */
	constexpr const float32 heightEpsilon = 0.1f;
	constexpr const std::array<float32, 3> rayHeightOffsets =
	{
		heightEpsilon,
		PLAYER_CAPSULE_HEIGHT * 0.5f,
		PLAYER_CAPSULE_HEIGHT - heightEpsilon
	};

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

	const QueryFilterData queryFilterData =
	{
		.IncludedGroup = FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
	};

	RaycastInfo raycastInfo =
	{
		.Origin = grenadePosition,
		.MaxDistance = GRENADE_BLAST_RADIUS,
		.pFilterData = &queryFilterData
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	for (Entity player : players)
	{
		const glm::vec3& playerPos = pPositionComponents->GetConstData(player).Position;

		for (float32 rayHeightOffset : rayHeightOffsets)
		{
			raycastInfo.Direction = glm::normalize(glm::vec3(playerPos.x, playerPos.y + rayHeightOffset, playerPos.z) - grenadePosition);

			constexpr const uint32 maxOverlaps = 10;
			std::array<PxRaycastHit, maxOverlaps> rayHitsUserBuffer;
			PxRaycastBuffer rayHits(rayHitsUserBuffer.data(), (PxU32)rayHitsUserBuffer.size());
			if (pPhysicsSystem->Raycast(raycastInfo, rayHits))
			{
				const uint32 hitCount = rayHits.getNbTouches();
				const PxRaycastHit* pHits = rayHits.getTouches();

				for (uint32 hitNr = 0; hitNr < hitCount; hitNr++)
				{
					const PxRaycastHit& hit = pHits[hitNr];
					if (reinterpret_cast<const ActorUserData*>(hit.actor->userData)->Entity == player)
					{
						// The player was hit by the ray, paint him in the hit position
						const LambdaEngine::EntityCollisionInfo collisionInfo0 =
						{
							.Entity		= grenadeEntity,
							.Position	= glm::vec3(hit.position.x, hit.position.y, hit.position.z),
							.Direction	= raycastInfo.Direction,
							.Normal		= glm::vec3(hit.normal.x, hit.normal.y, hit.normal.z)
						};

						const LambdaEngine::EntityCollisionInfo collisionInfo1 =
						{
							.Entity		= player,
							.Position	= glm::vec3(hit.position.x, hit.position.y, hit.position.z),
							.Direction	= raycastInfo.Direction,
							.Normal		= glm::vec3(hit.normal.x, hit.normal.y, hit.normal.z)
						};

						const EAmmoType ammoType	= EAmmoType::AMMO_TYPE_PAINT;
						const ETeam team			= (ETeam)grenadeTeam;
						const uint32 angle			= 0;

						ProjectileHitEvent hitEvent(collisionInfo0, collisionInfo1, ammoType, team, angle);
						EventQueue::SendEventImmediate(hitEvent);

						goto nextPlayer;
					}
				}
			}
		}
		nextPlayer: ;
	}
}

bool GrenadeSystem::OnPacketGrenadeThrownReceived(const PacketReceivedEvent<PacketGrenadeThrown>& grenadeThrownEvent)
{
	using namespace LambdaEngine;

	const Player* pThrowingPlayer = PlayerManagerBase::GetPlayer(grenadeThrownEvent.Packet.PlayerUID);
	Entity throwingPlayer = pThrowingPlayer->GetEntity();

	if (MultiplayerUtils::IsServer())
	{
		//Check if and spawn the grenade on the server
		if (TryThrowGrenade(throwingPlayer, grenadeThrownEvent.Packet.Position, grenadeThrownEvent.Packet.Velocity))
		{
			//If we successfully threw the grenade, tell all other players that we threw a grenade
			ServerHelper::SendToAllPlayers(grenadeThrownEvent.Packet, nullptr, pThrowingPlayer);
		}
	}
	else
	{
		//Trust the server blindly
		ThrowGrenade(throwingPlayer, grenadeThrownEvent.Packet.Position, grenadeThrownEvent.Packet.Velocity);
	}

	return true;
}
