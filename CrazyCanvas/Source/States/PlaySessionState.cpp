#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/Weapon.h"
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

#include "Game/Multiplayer/Client/ClientSystem.h"

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
	m_WeaponSystem.Init();

	ECSCore* pECS = ECSCore::GetInstance();

	ClientSystem& clientSystem = ClientSystem::GetInstance();
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PlaySessionState::OnPacketReceived);

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(2);
		MultiplayerUtils::RegisterClientEntityAccessor(m_pLevel);
	}
	
	//// Robot
	//{
	//	TArray<GUID_Lambda> animations;
	//	const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
	//	const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
	//	const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

	//	MaterialProperties materialProperties;
	//	materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//	materialProperties.Roughness = 1.0f;
	//	materialProperties.Metallic = 1.0f;

	//	const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
	//		"Robot Material",
	//		robotAlbedoGUID,
	//		robotNormalGUID,
	//		GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//		GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//		GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//		materialProperties);

	//	MeshComponent robotMeshComp = {};
	//	robotMeshComp.MeshGUID = robotGUID;
	//	robotMeshComp.MaterialGUID = robotMaterialGUID;

	//	AnimationComponent robotAnimationComp = {};
	//	robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
	//	robotAnimationComp.AnimationGUID = animations[0];
	//	robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;

	//	glm::vec3 position(0.0f, 1.25f, 0.0f);
	//	glm::vec3 scale(0.01f);

	//	Entity entity = pECS->CreateEntity();
	//	pECS->AddComponent<PositionComponent>(entity, { true, position });
	//	pECS->AddComponent<ScaleComponent>(entity, { true, scale });
	//	pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
	//	pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
	//	pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

	//	// Audio
	//	GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("halo_theme.wav");
	//	ISoundInstance3D* pSoundInstance = new SoundInstance3DFMOD(AudioAPI::GetDevice());
	//	const SoundInstance3DDesc desc =
	//	{
	//			.pName = "RobotSoundInstance",
	//			.pSoundEffect = ResourceManager::GetSoundEffect(soundGUID),
	//			.Flags = FSoundModeFlags::SOUND_MODE_NONE,
	//			.Position = position,
	//			.Volume = 0.03f
	//	};

	//	pSoundInstance->Init(&desc);
	//	pECS->AddComponent<AudibleComponent>(entity, { pSoundInstance });
	//}

	////Sphere Grid
	//{
	//	uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

	//	uint32 gridRadius = 5;

	//	for (uint32 y = 0; y < gridRadius; y++)
	//	{
	//		float32 roughness = y / float32(gridRadius - 1);

	//		for (uint32 x = 0; x < gridRadius; x++)
	//		{
	//			float32 metallic = x / float32(gridRadius - 1);

	//			MaterialProperties materialProperties;
	//			materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//			materialProperties.Roughness = roughness;
	//			materialProperties.Metallic = metallic;

	//			MeshComponent sphereMeshComp = {};
	//			sphereMeshComp.MeshGUID = sphereMeshGUID;
	//			sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
	//				"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
	//				GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
	//				GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//				GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//				GUID_TEXTURE_DEFAULT_COLOR_MAP,
	//				materialProperties);

	//			glm::vec3 position(-float32(gridRadius) * 0.5f + x, 2.0f + y, 5.0f);
	//			glm::vec3 scale(1.0f);

	//			Entity entity = pECS->CreateEntity();
	//			const CollisionInfo collisionCreateInfo = {
	//				.Entity			= entity,
	//				.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
	//				.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
	//				.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
	//				.Mesh			= pECS->AddComponent<MeshComponent>(entity, sphereMeshComp),
	//				.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
	//				.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
	//			};

	//			PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	//			StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticCollisionSphere(collisionCreateInfo);
	//			pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	//		}
	//	}
	//}
	TArray<GUID_Lambda> animations;
	ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);

	MaterialProperties materialProperties;
	materialProperties.Albedo = glm::vec4(1.0f);
	materialProperties.Roughness = 1.0f;
	materialProperties.Metallic = 1.0f;

	const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
		"Robot Material",
		ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true),
		ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true),
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		materialProperties);




	if (m_Online)
		ClientSystem::GetInstance().Connect(NetworkUtils::GetLocalAddress());
	else
		ClientSystem::GetInstance().Connect(IPAddress::LOOPBACK);
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
		bool animationsExist			= ResourceManager::GetAnimationGUIDsFromMeshName("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotGUID			= ResourceManager::GetMeshGUID("Robot/Rumba Dancing.fbx");
		const uint32 robotMaterialGUID	= ResourceManager::GetMaterialGUID("Robot Material");

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID		= robotGUID;
		robotMeshComp.MaterialGUID	= robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
		if (animationsExist) robotAnimationComp.Graph = AnimationGraph(AnimationState("dancing", animations[0]));

		/*Entity weaponEntity = pECS->CreateEntity();
		pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity, });*/

		CreatePlayerDesc createPlayerDesc =
		{
			.IsLocal = isLocal,
			.NetworkUID = networkUID,
			.pClient = event.pClient,
			.Position = position,
			.Forward = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
			.Scale = glm::vec3(1.0f),
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
