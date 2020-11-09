#include "States/BenchmarkState.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Debug/GPUProfiler.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineConfig.h"
#include "Events/GameplayEvents.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Input/API/Input.h"

#include "Utilities/RuntimeStats.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

BenchmarkState::BenchmarkState()
{
	using namespace LambdaEngine;
	SingleplayerInitializer::Init();
}

BenchmarkState::~BenchmarkState()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &BenchmarkState::OnWeaponFired);
	EventQueue::UnregisterEventHandler<NetworkSegmentReceivedEvent>(this, &BenchmarkState::OnPacketReceived);

	SAFEDELETE(m_pLevel);

	SingleplayerInitializer::Release();
}

void BenchmarkState::Init()
{
	using namespace LambdaEngine;
	Input::Disable();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	// Initialize event handlers
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();
	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &BenchmarkState::OnWeaponFired);
	EventQueue::RegisterEventHandler<NetworkSegmentReceivedEvent>(this, &BenchmarkState::OnPacketReceived);

	// Initialize Systems
	WeaponSystem::Init();
	m_BenchmarkSystem.Init();
	TrackSystem::GetInstance().Init();

	// Create camera with a track
	{
		const TArray<glm::vec3> cameraTrack = 
		{
			{-2.0f, 3.0f,  1.0f},
			{ 7.8f, 3.0f,  0.8f},
			{ 7.4f, 3.0f, -3.8f},
			{-7.8f, 4.0f, -3.9f},
			{-7.6f, 4.0f, -2.1f},
			{ 7.8f, 6.1f, -0.8f},
			{ 7.4f, 6.1f,  3.8f},
			{ 0.0f, 6.1f,  3.9f},
			{ 0.0f, 4.1f, -3.9f}
		};

		const CameraDesc cameraDesc = 
		{
			.FOVDegrees	= EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV),
			.Width		= (float32)window->GetWidth(),
			.Height		= (float32)window->GetHeight(),
			.NearPlane	= EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_NEAR_PLANE),
			.FarPlane	= EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FAR_PLANE)
		};

		m_Camera = CreateCameraTrackEntity(cameraDesc, cameraTrack);
	}

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
	}

	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		const float32 sphereRadius = PhysicsSystem::CalculateSphereRadius(ResourceManager::GetMesh(sphereMeshGUID));

		uint32 gridRadius = 5;

		for (uint32 y = 0; y < gridRadius; y++)
		{
			const float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				const float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo		= glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness	= roughness;
				materialProperties.Metallic		= metallic;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID		= sphereMeshGUID;
				sphereMeshComp.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				glm::vec3 position(-float32(gridRadius) * 0.5f + x, 2.0f + y, 5.0f);
				glm::vec3 scale(1.0f);

				Entity entity = pECS->CreateEntity();
				pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
				const CollisionCreateInfo collisionCreateInfo =
				{
					.Entity			= entity,
					.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
					.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
					.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
					.Shapes =
					{
						{
							.ShapeType		= EShapeType::SIMULATION,
							.GeometryType	= EGeometryType::SPHERE,
							.GeometryParams	= { .Radius = sphereRadius },
							.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
							.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
							.EntityID		= entity,
						},
					},
				};

				const StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
				pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);

				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
				transform *= glm::toMat4(glm::identity<glm::quat>());
				transform = glm::scale(transform, scale);
			}
		}

		// Directional Light
	/*	{
			m_DirLight = ECSCore::GetInstance()->CreateEntity();
			ECSCore::GetInstance()->AddComponent<PositionComponent>(m_DirLight, { { 0.0f, 0.0f, 0.0f} });
			ECSCore::GetInstance()->AddComponent<RotationComponent>(m_DirLight, { glm::quatLookAt({1.0f, -1.0f, 0.0f}, g_DefaultUp), true });
			ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(m_DirLight, DirectionalLightComponent{ .ColorIntensity = {1.0f, 1.0f, 1.0f, 5.0f} });
		}*/

		// Add PointLights
		{
			constexpr uint32 POINT_LIGHT_COUNT = 3;
			const PointLightComponent pointLights[POINT_LIGHT_COUNT] =
			{
				{.ColorIntensity = {1.0f, 0.0f, 0.0f, 25.0f}, .FarPlane = 25.f},
				{.ColorIntensity = {0.0f, 1.0f, 0.0f, 25.0f}, .FarPlane = 25.f},
				{.ColorIntensity = {0.0f, 0.0f, 1.0f, 25.0f}, .FarPlane = 25.f},
			};

			const glm::vec3 startPosition[3] =
			{
				{4.0f, 2.0f, 5.0f},
				{-4.0f, 2.0f, 5.0f},
				{0.0f, 2.0f, 5.0f},
			};

			for (uint32 i = 0; i < POINT_LIGHT_COUNT; i++)
			{
				TArray<glm::vec3> lightPath;
				lightPath.Reserve(7);

				lightPath.PushBack(startPosition[i]);
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 5.0f, 0.0f));
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 6.0f, 3.0f));
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 7.0f, -5.0f));
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 6.0f, -8.0f));
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 5.0f, -10.0f));
				lightPath.PushBack(startPosition[i] + glm::vec3(0.0f, 2.0f, -10.0f));

				MaterialProperties materialProperties;
				glm::vec3 color = pointLights[i].ColorIntensity;
				materialProperties.Albedo = glm::vec4(color, 1.0f);
				materialProperties.Roughness = 0.1f;
				materialProperties.Metallic = 0.1f;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				m_PointLights[i] = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(m_PointLights[i], { true, startPosition[i] });
				pECS->AddComponent<ScaleComponent>(m_PointLights[i], { true, glm::vec3(0.4f) });
				pECS->AddComponent<RotationComponent>(m_PointLights[i], { true, glm::identity<glm::quat>() });
				pECS->AddComponent<PointLightComponent>(m_PointLights[i], pointLights[i]);
				pECS->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
				pECS->AddComponent<TrackComponent>(m_PointLights[i], TrackComponent{ .Track = lightPath });
			}
		}
	}

	//Mirror
	{
		MaterialProperties mirrorProperties = {};
		mirrorProperties.Roughness = 0.0f;

		MeshComponent meshComponent;
		meshComponent.MeshGUID		= GUID_MESH_QUAD;
		meshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Mirror Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			mirrorProperties);

		Entity entity = ECSCore::GetInstance()->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, {0.0f, 3.0f, -7.0f} });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))) });
		pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.5f) });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	}

	// Triggers OnPacketReceived, which creates players
	SingleplayerInitializer::Setup();
}

void BenchmarkState::Tick(LambdaEngine::Timestamp delta)
{
	LambdaEngine::GPUProfiler::Get()->Tick(delta);

	if (LambdaEngine::TrackSystem::GetInstance().HasReachedEnd(m_Camera))
	{
		PrintBenchmarkResults();
		LambdaEngine::CommonApplication::Get()->Terminate();
	}
}

void BenchmarkState::FixedTick(LambdaEngine::Timestamp delta)
{
	WeaponSystem::GetInstance().FixedTick(delta);
}

bool BenchmarkState::OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event)
{
	using namespace LambdaEngine;

	if (event.Type == PacketType::CREATE_LEVEL_OBJECT)
	{
		PacketCreateLevelObject packet;
		event.pPacket->Read(&packet);

		// Create player characters that a benchmark system controls
		if (packet.LevelObjectType == ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER)
		{
			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

			const CameraDesc cameraDesc =
			{
				.FOVDegrees = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV),
				.Width		= (float32)window->GetWidth(),
				.Height		= (float32)window->GetHeight(),
				.NearPlane	= EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_NEAR_PLANE),
				.FarPlane	= EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FAR_PLANE)
			};

			const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx");
			TArray<GUID_Lambda> animations = ResourceManager::LoadAnimationsFromFile("Robot/Standard Walk.fbx");

			AnimationComponent robotAnimationComp = {};
			robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;

			constexpr const uint32 playerCount = 9;

			CreatePlayerDesc createPlayerDesc =
			{
				.ClientUID			= event.pClient->GetUID(),
				.IsLocal 			= false,
				.PlayerNetworkUID 	= packet.NetworkUID,
				.WeaponNetworkUID	= packet.Player.WeaponNetworkUID,
				.Position 			= packet.Position,
				.Forward 			= glm::normalize(glm::vec3(0.1f, -1.0f, 0.0f)),
				.Scale 				= glm::vec3(1.0f),
				.TeamIndex 			= 0,
				.pCameraDesc 		= &cameraDesc
			};

			for (uint32 playerNr = 0; playerNr < playerCount; playerNr++)
			{
				// Create a 3x3 grid of players in the XZ plane
				createPlayerDesc.Position.x			= -3.0f + 3.0f * (playerNr % 3);
				createPlayerDesc.Position.z			= -3.0f + 3.0f * (playerNr / 3);
				createPlayerDesc.Position.y			= 5.0f;
				createPlayerDesc.PlayerNetworkUID	+= 2;
				createPlayerDesc.WeaponNetworkUID	+= 2;

				TArray<Entity> createdPlayerEntities;
				if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
				{
					LOG_ERROR("[BenchmarkState]: Failed to create Player!");
				}
			}

			return true;
		}
	}

	return false;
}

bool BenchmarkState::OnWeaponFired(const WeaponFiredEvent& event)
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
		LOG_ERROR("Failed to create projectile");
	}

	return true;
}

void BenchmarkState::PrintBenchmarkResults()
{
	using namespace rapidjson;
	using namespace LambdaEngine;

	constexpr const float32 MB = 1024.0f * 1024.0f;

	const GPUProfiler* pGPUProfiler = GPUProfiler::Get();

	StringBuffer jsonStringBuffer;
	PrettyWriter<StringBuffer> writer(jsonStringBuffer);

	writer.StartObject();

	writer.String("AverageFPS");
	writer.Double(1.0f / RuntimeStats::GetAverageFrametime());
	writer.String("PeakRAM");
	writer.Double(RuntimeStats::GetPeakMemoryUsage() / MB);
	writer.String("PeakVRAM");
	writer.Double(pGPUProfiler->GetPeakDeviceMemory() / MB);
	writer.String("AverageVRAM");
	writer.Double(pGPUProfiler->GetAverageDeviceMemory() / MB);

	writer.EndObject();

	FILE* pFile = fopen("benchmark_results.json", "w");
	if (pFile)
	{
		fputs(jsonStringBuffer.GetString(), pFile);
		fclose(pFile);
	}
}
