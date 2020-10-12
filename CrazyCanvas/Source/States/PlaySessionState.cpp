#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineConfig.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"


#include "World/LevelManager.h"
#include "World/Level.h"

#include "Game/ECS/Systems/Networking/Client/ClientSystem.h"

#include "Application/API/Events/EventQueue.h"

PlaySessionState::PlaySessionState(bool online) : 
	m_Online(online)
{

}

PlaySessionState::~PlaySessionState()
{
	SAFEDELETE(m_pLevel);
}

void PlaySessionState::Init()
{
	using namespace LambdaEngine;

	ClientSystem& clientSystem = ClientSystem::GetInstance();
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PlaySessionState::OnPacketReceived);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
		MultiplayerUtils::RegisterClientEntityAccessor(m_pLevel);
	}

	if (m_Online)
		ClientSystem::GetInstance().Connect(NetworkUtils::GetLocalAddress());
	else
		ClientSystem::GetInstance().Connect(IPAddress::LOOPBACK);
	
	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

		uint32 gridRadius = 5;

		for (uint32 y = 0; y < gridRadius; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness = roughness;
				materialProperties.Metallic = metallic;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
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
				const StaticCollisionInfo collisionCreateInfo = {
					.Entity			= entity,
					.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
					.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
					.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
					.Mesh			= pECS->AddComponent<MeshComponent>(entity, sphereMeshComp),
					.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
					.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
				};

				pPhysicsSystem->CreateCollisionSphere(collisionCreateInfo);
			}
		}
	}
}

bool PlaySessionState::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	using namespace LambdaEngine;

	if (event.Type == NetworkSegment::TYPE_ENTITY_CREATE)
	{
		BinaryDecoder decoder(event.pPacket);
		bool isLocal = decoder.ReadBool();
		int32 networkUID = decoder.ReadInt32();
		glm::vec3 position = decoder.ReadVec3();

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

		const CameraDesc cameraDesc =
		{
			.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV"),
			.Width = (float)window->GetWidth(),
			.Height = (float)window->GetHeight(),
			.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane"),
			.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane")
		};

		TArray<GUID_Lambda> animations;
		const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f);
		materialProperties.Roughness = 1.0f;
		materialProperties.Metallic = 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID = robotGUID;
		robotMeshComp.MaterialGUID = robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
		robotAnimationComp.AnimationGUID = animations[0];

		CreatePlayerDesc createPlayerDesc =
		{
			.IsLocal = isLocal,
			.NetworkUID = networkUID,
			.pClient = event.pClient,
			.Position = position,
			.Forward = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
			.Scale = glm::vec3(0.01f),
			.pCameraDesc = &cameraDesc,
			.MeshComponent = robotMeshComp,
			.AnimationComponent = robotAnimationComp,
		};

		m_pLevel->CreateObject(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, &createPlayerDesc);

		return true;
	}

	return false;
}

void PlaySessionState::Tick(LambdaEngine::Timestamp)
{
}
