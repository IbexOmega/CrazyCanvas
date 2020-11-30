#include "World/LevelObjectCreator.h"
#include "World/Level.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Game/ECS/Components/Audio/ListenerComponent.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Player/PlayerRelatedComponent.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Components/Rendering/RayTracedComponent.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "ECS/ECSCore.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Match/ShowerSystemBase.h"
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Match/ShowerComponent.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/GUI/ProjectedGUIComponent.h"
#include "ECS/Components/Misc/DestructionComponent.h"

#include "Teams/TeamHelper.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"

#include "Math/Math.h"
#include "Math/Random.h"

#include "Resources/ResourceManager.h"

#include "Rendering/EntityMaskManager.h"

#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketPlayerAction.h"
#include "Multiplayer/Packet/PacketPlayerActionResponse.h"
#include "Multiplayer/Packet/PacketFlagEdited.h"
#include "Multiplayer/Packet/PacketHealthChanged.h"
#include "Multiplayer/Packet/PacketWeaponFired.h"
#include "Multiplayer/Packet/PacketResetPlayerTexture.h"

#include "Physics/CollisionGroups.h"

#include "Lobby/PlayerManagerBase.h"
#include "Lobby/PlayerManagerClient.h"

#include "Resources/ResourceCatalog.h"

#include "Match/Match.h"

bool LevelObjectCreator::Init()
{
	using namespace LambdaEngine;

	//Register Create Level Object by Prefix Functions
	{
		//Spawnpoint
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_PLAYER_SPAWN_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreatePlayerSpawn;
		}

		//Jailpoint
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_PLAYER_JAIL_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreatePlayerJail;
		}

		//Flag Spawn
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_FLAG_SPAWN_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateFlagSpawn;
		}

		//Flag Delivery Point
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_FLAG_DELIVERY_POINT_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateFlagDeliveryPoint;
		}

		//Kill plane
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_KILL_PLANE_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateKillPlane;
		}

		//Shower
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_PARTICLE_SHOWER_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateShowerPoint;
		}
	}

	//Register Create Special Object by Type Functions
	{
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG]		= &LevelObjectCreator::CreateFlag;
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER]		= &LevelObjectCreator::CreatePlayer;
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE]	= &LevelObjectCreator::CreateProjectile;
	}

	return true;
}

LambdaEngine::Entity LevelObjectCreator::CreateDirectionalLight(
	const LambdaEngine::LoadedDirectionalLight& directionalLight,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Entity entity = UINT32_MAX;

	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		DirectionalLightComponent directionalLightComponent =
		{
			.ColorIntensity = directionalLight.ColorIntensity,
		};

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, (translation) });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::quatLookAt({directionalLight.Direction}, g_DefaultUp) });
		pECS->AddComponent<DirectionalLightComponent>(entity, directionalLightComponent);

		LOG_DEBUG("Created Directional Light");
	}

	return entity;
}

LambdaEngine::Entity LevelObjectCreator::CreatePointLight(const LambdaEngine::LoadedPointLight& pointLight, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Entity entity = UINT32_MAX;

	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		PointLightComponent pointLightComponent =
		{
			.ColorIntensity = pointLight.ColorIntensity
		};

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, (pointLight.Position + translation) });
		pECS->AddComponent<PointLightComponent>(entity, pointLightComponent);

		LOG_DEBUG("Created Point Light");
	}

	return entity;
}

LambdaEngine::Entity LevelObjectCreator::CreateStaticGeometry(const LambdaEngine::MeshComponent& meshComponent, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);

	const float32 maxDim = glm::max<float32>(
		pMesh->DefaultScale.x * pMesh->BoundingBox.Dimensions.x,
		glm::max<float32>(
			pMesh->DefaultScale.y * pMesh->BoundingBox.Dimensions.y,
			pMesh->DefaultScale.z * pMesh->BoundingBox.Dimensions.z));

	const uint32 meshPaintSize = glm::max<uint32>(1, uint32(maxDim * 384.0f));

	ECSCore* pECS					= ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();
	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", meshPaintSize, meshPaintSize, false));
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<RayTracedComponent>(entity,
			RayTracedComponent
			{
				.HitMask = 0xFF
			});
	}

	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity		= entity,
		.Position	= pECS->AddComponent<PositionComponent>(entity, { true, pMesh->DefaultPosition + translation }),
		.Scale		= pECS->AddComponent<ScaleComponent>(entity,	{ true, pMesh->DefaultScale }),
		.Rotation	= pECS->AddComponent<RotationComponent>(entity, { true, pMesh->DefaultRotation }),
		.Shapes =
		{
			{
				/* ShapeType */			EShapeType::SIMULATION,
				/* GeometryType */		EGeometryType::MESH,
				/* Geometry */			{ .pMesh = pMesh },
				/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_STATIC,
				/* CollisionMask */		~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
				/* EntityID*/			entity
			},
		},
	};

	StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	return entity;
}

ELevelObjectType LevelObjectCreator::CreateLevelObjectFromPrefix(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	auto createFuncIt = s_LevelObjectByPrefixCreateFunctions.find(levelObject.Prefix);

	if (createFuncIt != s_LevelObjectByPrefixCreateFunctions.end())
	{
		return createFuncIt->second(levelObject, createdEntities, translation);
	}
	else
	{
		LOG_ERROR("Failed to create special object %s with prefix %s, no create function could be found", levelObject.Name.c_str(), levelObject.Prefix.c_str());
		return ELevelObjectType::LEVEL_OBJECT_TYPE_NONE;
	}
}

bool LevelObjectCreator::CreateLevelObjectOfType(
	ELevelObjectType levelObjectType,
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities)
{
	using namespace LambdaEngine;

	auto createFuncIt = s_LevelObjectByTypeCreateFunctions.find(levelObjectType);

	TArray<TArray<std::tuple<String, bool, Entity>>> createdChildEntities;

	if (createFuncIt != s_LevelObjectByTypeCreateFunctions.end())
	{
		bool success = createFuncIt->second(pData, createdEntities, createdChildEntities);

		if (success)
		{
			if (!createdChildEntities.IsEmpty())
			{
				ECSCore* pECS = ECSCore::GetInstance();

				for (uint32 e = 0; e < createdEntities.GetSize(); e++)
				{
					Entity parentEntity = createdEntities[e];
					ChildComponent childComponent = {};
					//If you crash here you havent pushed child entities for all entities created, its either all or none
					TArray<std::tuple<String, bool, Entity>>& childEntities = createdChildEntities[e];

					for (std::tuple<String, bool, Entity>& childEntityTuple : childEntities)
					{
						const String&	childTag		= std::get<0>(childEntityTuple);
						Entity			childEntity		= std::get<2>(childEntityTuple);

						childComponent.AddChild(childTag, childEntity, true);
					}

					pECS->AddComponent<ChildComponent>(parentEntity, childComponent);
				}
			}
		}

		return success;
	}
	else
	{
		LOG_ERROR("Failed to create special object, no create function could be found");
		return false;
	}
}

ELevelObjectType LevelObjectCreator::CreatePlayerSpawn(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	if (levelObject.BoundingBoxes.GetSize() > 1 )
	{
		LOG_WARNING("Player Spawn can currently not be created with more than one mesh, using the first mesh...");
	}

	ECSCore* pECS = ECSCore::GetInstance();
	Entity entity = pECS->CreateEntity();

	PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	TeamComponent teamComponent = {};

	if (!FindTeamIndex(levelObject.Name, teamComponent.TeamIndex))
	{
		LOG_ERROR("Team Index not found for Player Spawn, defaulting to 0...");
		teamComponent.TeamIndex = 0;
	}

	pECS->AddComponent<TeamComponent>(entity, teamComponent);

	if (!levelObject.MeshComponents.IsEmpty())
	{
		const MeshComponent& meshComponent = levelObject.MeshComponents[0];
		if (!MultiplayerUtils::IsServer())
		{
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
			pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 256, 256, false));
			pECS->AddComponent<RayTracedComponent>(entity, RayTracedComponent{
					.HitMask = 0xFF
				});
		}

		PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();
		const CollisionCreateInfo collisionCreateInfo =
		{
			.Entity			= entity,
			.Position		= positionComponent,
			.Scale			= scaleComponent,
			.Rotation		= rotationComponent,
			.Shapes =
			{
				{
					/* ShapeType */			EShapeType::SIMULATION,
					/* GeometryType */		EGeometryType::MESH,
					/* Geometry */			{ .pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID) },
					/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_STATIC,
					/* CollisionMask */		~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
					/* EntityID*/			entity
				},
			},
		};

		StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
		pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	}

	createdEntities.PushBack(entity);

	LOG_DEBUG("Created Player Spawn with EntityID %u and Team Index %u", entity, teamComponent.TeamIndex);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreatePlayerJail(const LambdaEngine::LevelObjectOnLoad& levelObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	if (levelObject.BoundingBoxes.GetSize() > 1)
	{
		LOG_WARNING("Player Jail can currently not be created with more than one mesh, using the first mesh...");
	}

	ECSCore* pECS = ECSCore::GetInstance();
	Entity entity = pECS->CreateEntity();

	PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	if (!levelObject.MeshComponents.IsEmpty())
	{
		const MeshComponent& meshComponent = levelObject.MeshComponents[0];

		pECS->AddComponent<MeshComponent>(entity, meshComponent);

		PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

		const CollisionCreateInfo collisionCreateInfo =
		{
			.Entity = entity,
			.Position = positionComponent,
			.Scale = scaleComponent,
			.Rotation = rotationComponent,
			.Shapes =
			{
				{
					/* ShapeType */			EShapeType::SIMULATION,
					/* GeometryType */		EGeometryType::MESH,
					/* Geometry */			{.pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID) },
					/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_STATIC,
					/* CollisionMask */		~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
					/* EntityID*/			entity
				},
			},
		};

		StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
		pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	}

	createdEntities.PushBack(entity);

	LOG_DEBUG("Created Player Jail with EntityID %u", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_JAIL;
}

ELevelObjectType LevelObjectCreator::CreateFlagSpawn(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagSpawnComponent>(entity, FlagSpawnComponent());
	pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });

	uint8 teamIndex = 0;
	if (FindTeamIndex(levelObject.Name, teamIndex))
	{
		pECS->AddComponent<TeamComponent>(entity, { .TeamIndex = teamIndex });
	}

	createdEntities.PushBack(entity);

	LOG_DEBUG("Created Flag Spawn with EntityID %u", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreateFlagDeliveryPoint(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;
	//Only the server is allowed to create a Base
	if (!MultiplayerUtils::IsServer())
		return ELevelObjectType::LEVEL_OBJECT_TYPE_NONE;

	if (levelObject.BoundingBoxes.GetSize() > 1)
	{
		LOG_WARNING("Bases can currently not be created with more than one Bounding Box, using the first Bounding Box...");
	}

	const BoundingBox& boundingBox = levelObject.BoundingBoxes[0];

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagDeliveryPointComponent>(entity, {});

	TeamComponent teamComponent = {};

	if (!FindTeamIndex(levelObject.Name, teamComponent.TeamIndex))
	{
		LOG_ERROR("Team Index not found for Flag Delivery Point, defaulting to 0...");
		teamComponent.TeamIndex = 0;
	}

	pECS->AddComponent<TeamComponent>(entity, teamComponent);
	const PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	const ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	const RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	//Only the server checks collision with the flag
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity		= entity,
		.Position	= positionComponent,
		.Scale		= scaleComponent,
		.Rotation	= rotationComponent,
		.Shapes =
		{
			{
				/* Shape Type */		EShapeType::TRIGGER,
				/* GeometryType */		EGeometryType::BOX,
				/* Geometry */			{ .HalfExtents = boundingBox.Dimensions },
				/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG_DELIVERY_POINT,
				/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
				/* EntityID*/			entity
			},
		},
	};

	StaticCollisionComponent collisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, collisionComponent);

	createdEntities.PushBack(entity);

	LOG_DEBUG("Created Base with EntityID %u and Team Index %u", entity, teamComponent.TeamIndex);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_DELIVERY_POINT;
}

ELevelObjectType LevelObjectCreator::CreateKillPlane(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	const Entity entity = pECS->CreateEntity();
	const glm::vec3 boxHalfExtents = levelObject.BoundingBoxes.GetBack().Dimensions * levelObject.DefaultScale * 0.5f;

	const CollisionCreateInfo colliderInfo =
	{
		.Entity		= entity,
		.Position	= pECS->AddComponent<PositionComponent>(entity,	{ true, levelObject.DefaultPosition + translation }),
		.Scale		= pECS->AddComponent<ScaleComponent>(entity,	{ true, levelObject.DefaultScale }),
		.Rotation	= pECS->AddComponent<RotationComponent>(entity,	{ true, glm::identity<glm::quat>() }),
		.Shapes	=
		{
			{
				.ShapeType			= EShapeType::TRIGGER,
				.GeometryType		= EGeometryType::PLANE,
				.CollisionGroup		= FCollisionGroup::COLLISION_GROUP_STATIC,
				.CollisionMask		= ~FCollisionGroup::COLLISION_GROUP_STATIC,
				.EntityID			= entity,
				.CallbackFunction	= &Match::KillPlaneCallback
			}
		}
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	const StaticCollisionComponent staticCollider = pPhysicsSystem->CreateStaticActor(colliderInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollider);
	createdEntities.PushBack(entity);

	LOG_DEBUG("Created Kill Plane with EntityID %u", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_KILL_PLANE;
}

ELevelObjectType LevelObjectCreator::CreateShowerPoint(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Entity entity = pECS->CreateEntity();

	const PositionComponent& positionComponent	= pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	const ScaleComponent& scaleComponent		= pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	const RotationComponent& rotationComponent	= pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<ParticleEmitterComponent>(entity,
			ParticleEmitterComponent{
				.Active = true,
				.OneTime = false,
				.Explosive = 0.5f,
				.SpawnDelay = 0.05f,
				.ParticleCount = 512,
				.EmitterShape = EEmitterShape::CONE,
				.Angle = 45.f,
				.VelocityRandomness = 0.5f,
				.Velocity = 2.0,
				.Acceleration = 0.0,
				.Gravity = -7.f,
				.LifeTime = 1.2f,
				.RadiusRandomness = 0.5f,
				.BeginRadius = 0.2f,
				.FrictionFactor = 0.f,
				.Bounciness = 0.f,
				.RandomStartIndex = true,
				.AnimationCount = 1,
				.FirstAnimationIndex = 6,
				.Color = glm::vec4(0.0f, 0.5f, 1.0f, 1.f)
			}
		);
	}
	else
	{
		const BoundingBox& boundingBox = levelObject.BoundingBoxes[0];
		const CollisionCreateInfo collisionCreateInfo =
		{
			.Entity		= entity,
			.Position	= positionComponent,
			.Scale		= scaleComponent,
			.Rotation	= rotationComponent,
			.Shapes =
			{
				{
					/* Shape Type */		EShapeType::TRIGGER,
					/* GeometryType */		EGeometryType::BOX,
					/* Geometry */			{.HalfExtents = boundingBox.Dimensions },
					/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_SHOWER,
					/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
					/* EntityID*/			entity,
					/* CallbackFunction*/	&ShowerSystemBase::PlayerShowerCollision
				}
			},
		};

		PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
		const StaticCollisionComponent staticCollider = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
		pECS->AddComponent<StaticCollisionComponent>(entity, staticCollider);
	}

	createdEntities.PushBack(entity);
	LOG_DEBUG("Created Particle Shower with EntityID %u", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_PARTICLE_SHOWER;
}

bool LevelObjectCreator::CreateFlag(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities)
{
	UNREFERENCED_VARIABLE(createdChildEntities);

	if (pData == nullptr) return false;

	using namespace LambdaEngine;

	const CreateFlagDesc* pFlagDesc = reinterpret_cast<const CreateFlagDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity flagEntity = pECS->CreateEntity();

	GUID_Lambda flagMaterialGUID = ResourceCatalog::FLAG_COMMON_MATERIAL_GUID;

	if (pFlagDesc->TeamIndex != UINT8_MAX)
	{
		pECS->AddComponent<TeamComponent>(flagEntity, { .TeamIndex = pFlagDesc->TeamIndex });
		flagMaterialGUID = TeamHelper::GetTeamColorMaterialGUID(pFlagDesc->TeamIndex);
	}

	const Timestamp			pickupCooldown = Timestamp::Seconds(1.0f);
	const Timestamp			respawnCooldown = Timestamp::Seconds(5.0f);
	const FlagComponent		flagComponent{ EngineLoop::GetTimeSinceStart(), pickupCooldown, respawnCooldown, false };
	const PositionComponent	positionComponent{ true, pFlagDesc->Position };
	const ScaleComponent	scaleComponent{ true, pFlagDesc->Scale };
	const RotationComponent	rotationComponent{ true, pFlagDesc->Rotation };
	const MeshComponent		meshComponent{ ResourceCatalog::FLAG_MESH_GUID, flagMaterialGUID };
	const ProjectedGUIComponent	projectedGUIComponent{ IndicatorTypeGUI::FLAG_INDICATOR };

	pECS->AddComponent<FlagComponent>(flagEntity,			flagComponent);
	pECS->AddComponent<PositionComponent>(flagEntity,		positionComponent);
	pECS->AddComponent<ScaleComponent>(flagEntity,			scaleComponent);
	pECS->AddComponent<RotationComponent>(flagEntity,		rotationComponent);
	pECS->AddComponent<MeshComponent>(flagEntity,			meshComponent);
	pECS->AddComponent<ProjectedGUIComponent>(flagEntity,	projectedGUIComponent);

	bool attachedToParent = pFlagDesc->ParentEntity != UINT32_MAX;

	ParentComponent parentComponent =
	{
		.Parent					= pFlagDesc->ParentEntity,
		.Attached				= attachedToParent,
		.DeleteParentOnRemoval	= false
	};

	OffsetComponent offsetComponent =
	{
		.Offset		= glm::vec3(0.0f)
	};

	if (attachedToParent)
	{
		const CharacterColliderComponent& parentCollisionComponent = pECS->GetConstComponent<CharacterColliderComponent>(pFlagDesc->ParentEntity);

		//Set Flag Offset
		const physx::PxBounds3& parentBoundingBox = parentCollisionComponent.pController->getActor()->getWorldBounds();
		offsetComponent.Offset = glm::vec3(0.0f, parentBoundingBox.getDimensions().y / 2.0f, 0.0f);
	}

	pECS->AddComponent<ParentComponent>(flagEntity,	parentComponent);
	pECS->AddComponent<OffsetComponent>(flagEntity,	offsetComponent);

	pECS->AddComponent<PacketComponent<PacketFlagEdited>>(flagEntity, {});

	int32 networkUID;
	if (!MultiplayerUtils::IsServer())
	{
		networkUID = pFlagDesc->NetworkUID;

		// Add Animation attachment on client
		pECS->AddComponent<AnimationAttachedComponent>(flagEntity, AnimationAttachedComponent
			{
				.JointName = "mixamorig:Spine2",
				.Transform = glm::mat4(1.0f),
			});
	}
	else
	{
		EFlagColliderType flagPlayerColliderType = EFlagColliderType::FLAG_COLLIDER_TYPE_PLAYER;
		EFlagColliderType flagDeliveryPointColliderType = EFlagColliderType::FLAG_COLLIDER_TYPE_DELIVERY_POINT;

		pECS->AddComponent<RayTracedComponent>(flagEntity, RayTracedComponent{
				.HitMask = 0xFF
			});

		//Only the server checks collision with the flag
		const Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);
		const DynamicCollisionCreateInfo collisionCreateInfo =
		{
			/* Entity */	 		flagEntity,
			/* Detection Method */	ECollisionDetection::DISCRETE,
			/* Position */	 		positionComponent,
			/* Scale */				scaleComponent,
			/* Rotation */			rotationComponent,
			{
				{	// Triggers Flag-Player Collisions
					/* Shape Type */		EShapeType::TRIGGER,
					/* GeometryType */		EGeometryType::BOX,
					/* Geometry */			{ .HalfExtents = pMesh->BoundingBox.Dimensions },
					/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
					/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
					/* EntityID*/			flagEntity,
					/* CallbackFunction */	&FlagSystemBase::StaticOnPlayerFlagCollision,
					/* UserData */			&flagPlayerColliderType,
					/* UserDataSize */		sizeof(EFlagColliderType)
				},
				{	// Triggers Flag-Delivery Point Collisions
					/* Shape Type */		EShapeType::SIMULATION,
					/* GeometryType */		EGeometryType::BOX,
					/* Geometry */			{ .HalfExtents = pMesh->BoundingBox.Dimensions },
					/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
					/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG_DELIVERY_POINT,
					/* EntityID*/			flagEntity,
					/* CallbackFunction */& FlagSystemBase::StaticOnDeliveryPointFlagCollision,
					/* UserData */			&flagDeliveryPointColliderType,
					/* UserDataSize */		sizeof(EFlagColliderType)
				},
			},
			/* Velocity */			pECS->AddComponent<VelocityComponent>(flagEntity, { glm::vec3(0.0f) })
		};

		DynamicCollisionComponent collisionComponent = pPhysicsSystem->CreateDynamicActor(collisionCreateInfo);
		collisionComponent.pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		pECS->AddComponent<DynamicCollisionComponent>(flagEntity, collisionComponent);

		networkUID = (int32)flagEntity;
	}

	pECS->AddComponent<NetworkComponent>(flagEntity, { networkUID });

	createdEntities.PushBack(flagEntity);

	LOG_DEBUG("Created Flag with EntityID %u and NetworkID %u", flagEntity, networkUID);
	return true;
}

bool LevelObjectCreator::CreatePlayer(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities)
{
	if (pData == nullptr)
		return false;

	using namespace LambdaEngine;

	const CreatePlayerDesc* pPlayerDesc = reinterpret_cast<const CreatePlayerDesc*>(pData);

	const Player* pPlayer = pPlayerDesc->pPlayer;

	ECSCore* pECS = ECSCore::GetInstance();
	const Entity playerEntity = pECS->CreateEntity();
	createdEntities.PushBack(playerEntity);
	TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>& childEntities = createdChildEntities.PushBack({});

	const glm::quat lookDirQuat = glm::quatLookAt(pPlayerDesc->Forward, g_DefaultUp);

	pECS->AddComponent<PlayerBaseComponent>(playerEntity,		PlayerBaseComponent());
	pECS->AddComponent<PlayerRelatedComponent>(playerEntity, PlayerRelatedComponent());
	EntityMaskManager::AddExtensionToEntity(playerEntity, PlayerRelatedComponent::Type(), nullptr);

	pECS->AddComponent<PositionComponent>(playerEntity,			PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<NetworkPositionComponent>(playerEntity,
		NetworkPositionComponent
		{
			.Position		= pPlayerDesc->Position,
			.PositionLast	= pPlayerDesc->Position,
			.TimestampStart = EngineLoop::GetTimeSinceStart(),
			.Duration		= EngineLoop::GetFixedTimestep()
		});

	pECS->AddComponent<RotationComponent>(playerEntity,			RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ScaleComponent>(playerEntity,			ScaleComponent{ .Scale = pPlayerDesc->Scale });
	pECS->AddComponent<VelocityComponent>(playerEntity,			VelocityComponent());
	pECS->AddComponent<TeamComponent>(playerEntity,				TeamComponent{ .TeamIndex = pPlayer->GetTeam() });
	pECS->AddComponent<PacketComponent<PacketPlayerAction>>(playerEntity, { });
	pECS->AddComponent<PacketComponent<PacketPlayerActionResponse>>(playerEntity, { });

	const CharacterColliderCreateInfo colliderInfo =
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask	= (uint32)FCollisionGroup::COLLISION_GROUP_STATIC |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_SHOWER |
						 (uint32)FCollisionGroup::COLLISION_GROUP_DYNAMIC,
		.EntityID		= playerEntity
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	CharacterColliderComponent characterColliderComponent = pPhysicsSystem->CreateCharacterCapsule(
		colliderInfo,
		std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS),
		PLAYER_CAPSULE_RADIUS);

	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);

	Entity weaponEntity = pECS->CreateEntity();
	childEntities.PushBack(std::make_tuple("weapon", true, weaponEntity));
	pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity });
	pECS->AddComponent<PacketComponent<PacketWeaponFired>>(weaponEntity, { });
	pECS->AddComponent<PositionComponent>(weaponEntity, PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<RotationComponent>(weaponEntity, RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ScaleComponent>(weaponEntity, ScaleComponent{ .Scale = glm::vec3(1.0f) });
	pECS->AddComponent<OffsetComponent>(weaponEntity, OffsetComponent{ .Offset = pPlayerDesc->Scale * glm::vec3(0.0f, 1.5f, 0.0f) });
	pECS->AddComponent<TeamComponent>(weaponEntity, TeamComponent{ .TeamIndex = pPlayer->GetTeam() });
	pECS->AddComponent<MeshPaintComponent>(weaponEntity, MeshPaint::CreateComponent(weaponEntity, "WeaponUnwrappedTexture", 256, 256, false));
	pECS->AddComponent<PlayerRelatedComponent>(weaponEntity, PlayerRelatedComponent{});
	EntityMaskManager::AddExtensionToEntity(weaponEntity, PlayerRelatedComponent::Type(), nullptr);

	const bool readback = MultiplayerUtils::IsServer();
	pECS->AddComponent<MeshPaintComponent>(playerEntity, MeshPaint::CreateComponent(playerEntity, "PlayerUnwrappedTexture", 512, 512, true, readback));

	AnimationComponent animationComponent = {};
	animationComponent.Pose.pSkeleton = ResourceManager::GetMesh(ResourceCatalog::PLAYER_MESH_GUID)->pSkeleton;

	AnimationGraph* pAnimationGraph = DBG_NEW AnimationGraph();
	pAnimationGraph->AddState(DBG_NEW AnimationState("Idle", ResourceCatalog::PLAYER_IDLE_GUIDs[0]));
	pAnimationGraph->TransitionToState("Idle");
	animationComponent.pGraph = pAnimationGraph;

	pECS->AddComponent<AnimationComponent>(playerEntity, animationComponent);

	GUID_Lambda playerMaterialGUID;

	// Server/Client
	int32 playerNetworkUID;
	int32 weaponNetworkUID;
	if (!MultiplayerUtils::IsServer())
	{
		playerMaterialGUID = PlayerManagerClient::GetPlayerLocal()->GetTeam() == pPlayer->GetTeam() ? TeamHelper::GetMyTeamPlayerMaterialGUID() : TeamHelper::GetTeamPlayerMaterialGUID(pPlayer->GetTeam());

		pECS->AddComponent<MeshComponent>(weaponEntity, MeshComponent
			{
				.MeshGUID = ResourceCatalog::WEAPON_MESH_GUID,
				.MaterialGUID = ResourceCatalog::WEAPON_MATERIAL_GUID,
			});

		pECS->AddComponent<RayTracedComponent>(weaponEntity, RayTracedComponent{
				.HitMask = 0xFF
			});

		pECS->AddComponent<AnimationAttachedComponent>(weaponEntity, AnimationAttachedComponent
			{
				.JointName	= "mixamorig:RightHand",
				.Transform	= glm::mat4(1.0f),
			});

		playerNetworkUID = pPlayerDesc->PlayerNetworkUID;
		weaponNetworkUID = pPlayerDesc->WeaponNetworkUID;

#ifdef USE_ALL_ANIMATIONS
		pAnimationGraph->AddState(DBG_NEW AnimationState("Running", ResourceCatalog::PLAYER_RUN_GUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Run Backward", ResourceCatalog::PLAYER_RUN_BACKWARD_GUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Strafe Right", ResourceCatalog::PLAYER_STRAFE_RIGHT_GUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Strafe Left", ResourceCatalog::PLAYER_STRAFE_LEFT_GUIDs[0]));

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Running & Strafe Left");
			ClipNode* pRunning = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_RUN_MIRRORED_GUIDs[0]);
			ClipNode* pStrafeLeft = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_STRAFE_LEFT_GUIDs[0]);
			BlendNode* pBlendNode = pAnimationState->CreateBlendNode(pStrafeLeft, pRunning, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Running & Strafe Right");
			ClipNode* pRunning = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_RUN_GUIDs[0]);
			ClipNode* pStrafeRight = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_STRAFE_RIGHT_GUIDs[0]);
			BlendNode* pBlendNode = pAnimationState->CreateBlendNode(pStrafeRight, pRunning, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Run Backward & Strafe Left");
			ClipNode* pRunningBackward = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_RUN_BACKWARD_MIRRORED_GUIDs[0]);
			ClipNode* pStrafeLeft = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_STRAFE_LEFT_GUIDs[0]);
			BlendNode* pBlendNode = pAnimationState->CreateBlendNode(pStrafeLeft, pRunningBackward, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Run Backward & Strafe Right");
			ClipNode* pRunningBackward = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_RUN_BACKWARD_GUIDs[0]);
			ClipNode* pStrafeRight = pAnimationState->CreateClipNode(ResourceCatalog::PLAYER_STRAFE_RIGHT_GUIDs[0]);
			BlendNode* pBlendNode = pAnimationState->CreateBlendNode(pStrafeRight, pRunningBackward, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Run Backward & Strafe Left"));
#endif
		pECS->AddComponent<RayTracedComponent>(playerEntity, RayTracedComponent{
				.HitMask = 0xFF
			});

		//Add Audio Instances
		{
			SoundInstance3DDesc soundInstanceDesc = {};
			soundInstanceDesc.pName			= "Step";
			soundInstanceDesc.pSoundEffect	= ResourceManager::GetSoundEffect3D(ResourceCatalog::PLAYER_STEP_SOUND_GUID);
			soundInstanceDesc.Flags			= FSoundModeFlags::SOUND_MODE_NONE;
			soundInstanceDesc.Position		= pPlayerDesc->Position;
			soundInstanceDesc.Volume		= 1.0f;

			AudibleComponent audibleComponent = {};
			audibleComponent.SoundInstances3D[soundInstanceDesc.pName] = AudioAPI::GetDevice()->Create3DSoundInstance(&soundInstanceDesc);

			pECS->AddComponent<AudibleComponent>(playerEntity, audibleComponent);
		}

		if (!pPlayerDesc->IsLocal)
		{
			pECS->AddComponent<PlayerForeignComponent>(playerEntity, PlayerForeignComponent());
		}
		else
		{
			if (pPlayerDesc->pCameraDesc == nullptr)
			{
				pECS->RemoveEntity(playerEntity);
				LOG_ERROR("Local Player must have a camera description");
				return false;
			}

			pECS->AddComponent<WeaponLocalComponent>(weaponEntity, WeaponLocalComponent());
			EntityMaskManager::AddExtensionToEntity(weaponEntity, WeaponLocalComponent::Type(), nullptr);

			pECS->AddComponent<PlayerLocalComponent>(playerEntity, PlayerLocalComponent());
			EntityMaskManager::AddExtensionToEntity(playerEntity, PlayerLocalComponent::Type(), nullptr);

			//Create Camera Entity
			Entity cameraEntity = pECS->CreateEntity();
			childEntities.PushBack(std::make_tuple("camera", true, cameraEntity));

			//Todo: Better implementation for this somehow maybe?
			const Mesh* pMesh = ResourceManager::GetMesh(ResourceCatalog::PLAYER_MESH_GUID);
			OffsetComponent offsetComponent = { .Offset = pPlayerDesc->Scale * glm::vec3(0.0f, 0.95f * pMesh->BoundingBox.Dimensions.y, 0.0f) };

			pECS->AddComponent<OffsetComponent>(cameraEntity, offsetComponent);
			pECS->AddComponent<PositionComponent>(cameraEntity, PositionComponent{ .Position = pPlayerDesc->Position + offsetComponent.Offset });
			pECS->AddComponent<ScaleComponent>(cameraEntity, ScaleComponent{ .Scale = {1.0f, 1.0f, 1.0f} });
			pECS->AddComponent<RotationComponent>(cameraEntity, RotationComponent{ .Quaternion = lookDirQuat });
			pECS->AddComponent<ListenerComponent>(cameraEntity, { AudioAPI::GetDevice()->GetAudioListener(false) });

			const ViewProjectionMatricesComponent viewProjComp =
			{
				.Projection = glm::perspective(
					glm::radians(pPlayerDesc->pCameraDesc->FOVDegrees),
					pPlayerDesc->pCameraDesc->Width / pPlayerDesc->pCameraDesc->Height,
					pPlayerDesc->pCameraDesc->NearPlane,
					pPlayerDesc->pCameraDesc->FarPlane),

				.View = glm::lookAt(
					pPlayerDesc->Position,
					pPlayerDesc->Position + pPlayerDesc->Forward,
					g_DefaultUp)
			};
			pECS->AddComponent<ViewProjectionMatricesComponent>(cameraEntity, viewProjComp);

			const CameraComponent cameraComp =
			{
				.NearPlane	= pPlayerDesc->pCameraDesc->NearPlane,
				.FarPlane	= pPlayerDesc->pCameraDesc->FarPlane,
				.FOV		= pPlayerDesc->pCameraDesc->FOVDegrees
			};
			pECS->AddComponent<CameraComponent>(cameraEntity, cameraComp);
			pECS->AddComponent<StepParentComponent>(cameraEntity, StepParentComponent{ .Owner = playerEntity});
		}
	}
	else
	{
		playerMaterialGUID = TeamHelper::GetMyTeamPlayerMaterialGUID();
		playerNetworkUID = (int32)playerEntity;
		weaponNetworkUID = (int32)weaponEntity;

		const Timestamp	showerCooldown = Timestamp::Seconds(5.0f);
		const ShowerComponent showerComponent{ EngineLoop::GetTimeSinceStart() + showerCooldown, showerCooldown };
		pECS->AddComponent<ShowerComponent>(playerEntity, showerComponent);
	}

	pECS->AddComponent<MeshComponent>(playerEntity,
		MeshComponent
		{
			.MeshGUID = ResourceCatalog::PLAYER_MESH_GUID,
			.MaterialGUID = playerMaterialGUID
		});

	pECS->AddComponent<NetworkComponent>(playerEntity, { playerNetworkUID });
	pECS->AddComponent<HealthComponent>(playerEntity, HealthComponent());
	pECS->AddComponent<PacketComponent<PacketHealthChanged>>(playerEntity, {});
	pECS->AddComponent<PacketComponent<PacketResetPlayerTexture>>(playerEntity, {});
	pECS->AddComponent<NetworkComponent>(weaponEntity, { weaponNetworkUID });

	PlayerManagerBase::SetPlayerEntity(pPlayer, playerEntity);

	LOG_DEBUG("Created Player with EntityID %d and NetworkID %d", playerEntity, playerNetworkUID);
	LOG_DEBUG("Created Weapon with EntityID %d and NetworkID %d", weaponEntity, weaponNetworkUID);

	return true;
}

bool LevelObjectCreator::CreateProjectile(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(createdChildEntities);

	if (pData == nullptr)
	{
		return false;
	}

	const CreateProjectileDesc& desc = *reinterpret_cast<const CreateProjectileDesc*>(pData);

	// Create a projectile entity
	ECSCore* pECS = ECSCore::GetInstance();
	const Entity projectileEntity = pECS->CreateEntity();
	createdEntities.PushBack(projectileEntity);

	const VelocityComponent velocityComponent = { desc.InitalVelocity };
	pECS->AddComponent<VelocityComponent>(projectileEntity, velocityComponent);

	ProjectileComponent projectileComp;
	projectileComp.AmmoType	= desc.AmmoType;
	projectileComp.Owner	= desc.WeaponOwner;
	projectileComp.Angle	= desc.Angle;
	pECS->AddComponent<ProjectileComponent>(projectileEntity, projectileComp);
	pECS->AddComponent<TeamComponent>(projectileEntity, { static_cast<uint8>(desc.TeamIndex) });

	const glm::vec3 normVelocity = glm::normalize(desc.InitalVelocity);
	const glm::vec3 projectileOffset = normVelocity * 0.5f;
	PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(projectileEntity, { true, desc.FirePosition + projectileOffset });
	ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(projectileEntity, { true, glm::vec3(0.7f) });
	RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(projectileEntity, { true, glm::quatLookAt(normVelocity, g_DefaultUp) });

	const DynamicCollisionCreateInfo collisionInfo =
	{
		/* Entity */	 		projectileEntity,
		/* Detection Method */	ECollisionDetection::CONTINUOUS,
		/* Position */	 		positionComponent,
		/* Scale */				scaleComponent,
		/* Rotation */			rotationComponent,
		{
			{
				/* Shape Type */		EShapeType::SIMULATION,
				/* Geometry Type */		EGeometryType::SPHERE,
				/* Geometry Params */	{ .Radius = 0.3f },
				/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_DYNAMIC,
				/* CollisionMask */		(uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
										(uint32)FCollisionGroup::COLLISION_GROUP_STATIC,
				/* EntityID*/			desc.WeaponOwner,
				/* CallbackFunction */	desc.Callback,
			},
		},
		/* Velocity */			velocityComponent
	};

	const DynamicCollisionComponent projectileCollisionComp = PhysicsSystem::GetInstance()->CreateDynamicActor(collisionInfo);
	pECS->AddComponent<DynamicCollisionComponent>(projectileEntity, projectileCollisionComp);

	if (!MultiplayerUtils::IsServer())
	{
		glm::vec4 particleColor(1.0f);
		if (desc.AmmoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			GUID_Lambda projectileMaterialGUID = TeamHelper::GetTeamColorMaterialGUID(desc.TeamIndex);
			pECS->AddComponent<MeshComponent>(projectileEntity, MeshComponent{ .MeshGUID = ResourceCatalog::PROJECTILE_MESH_GUID, .MaterialGUID = projectileMaterialGUID });
			particleColor = glm::vec4(TeamHelper::GetTeamColor(desc.TeamIndex), 1.0f);
		}
		else
		{
			pECS->AddComponent<MeshComponent>(projectileEntity, MeshComponent{ .MeshGUID = ResourceCatalog::PROJECTILE_MESH_GUID, .MaterialGUID = ResourceCatalog::PROJECTILE_WATER_MATERIAL });
			particleColor = glm::vec4(0.34, 0.85, 1.0f, 1.0f);
		}

		// Create particles
		ParticleEmitterComponent emitterComponent = ParticleEmitterComponent{
				.Active = true,
				.OneTime = false,
				.Explosive = 0.5f,
				.SpawnDelay = 0.05f,
				.ParticleCount = 64,
				.EmitterShape = EEmitterShape::CONE,
				.Angle = 90.f,
				.VelocityRandomness = 0.5f,
				.Velocity = 1.0,
				.Acceleration = 0.0,
				.Gravity = -7.f,
				.LifeTime = 3.0f,
				.RadiusRandomness = 0.5f,
				.BeginRadius = 0.2f,
				.FrictionFactor = 0.f,
				.Bounciness = 0.f,
				.RandomStartIndex = true,
				.AnimationCount = 4,
				.FirstAnimationIndex = 0,
				.Color = particleColor,
		};

		// Create trail particles
		pECS->AddComponent<ParticleEmitterComponent>(projectileEntity, emitterComponent);

		// Create muzzle particles
		{
			emitterComponent.OneTime = true;
			emitterComponent.ParticleCount = 64;
			emitterComponent.BeginRadius = 0.1f;
			emitterComponent.Explosive = 1.0f;
			emitterComponent.SpawnDelay = 0.1f;
			emitterComponent.Velocity = 6.0f;
			emitterComponent.Angle = 45.0f;

			const Entity particleEntity = pECS->CreateEntity();
			pECS->AddComponent<PositionComponent>(particleEntity, { true, desc.FirePosition + projectileOffset });
			pECS->AddComponent<ScaleComponent>(particleEntity, { true, glm::vec3(0.7f) });
			pECS->AddComponent<RotationComponent>(particleEntity, { true, glm::quatLookAt(normVelocity, g_DefaultUp) });
			pECS->AddComponent<DestructionComponent>(particleEntity, { .TimeLeft = 0.1f });
			pECS->AddComponent<ParticleEmitterComponent>(particleEntity, emitterComponent);
		}

		pECS->AddComponent<RayTracedComponent>(projectileEntity, RayTracedComponent{
				.HitMask = 0x02
			});
	}

	return true;
}

bool LevelObjectCreator::FindTeamIndex(const LambdaEngine::String& objectName, uint8& teamIndex)
{
	using namespace LambdaEngine;

	size_t teamIndexPos = objectName.find("TEAM");
	if (teamIndexPos != String::npos)
	{
		teamIndex = (uint8)std::stoi(objectName.substr(teamIndexPos + 4));
		return true;
	}

	return false;
}
