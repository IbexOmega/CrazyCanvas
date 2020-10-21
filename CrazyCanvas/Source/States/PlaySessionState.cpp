#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
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

PlaySessionState::PlaySessionState(LambdaEngine::IPAddress* pIPAddress) :
	m_pIPAddress(pIPAddress)
{

}

PlaySessionState::~PlaySessionState()
{
	SAFEDELETE(m_pLevel);
}

void PlaySessionState::Init()
{
	using namespace LambdaEngine;

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();

	m_WeaponSystem.Init();
	m_HUDSystem.Init();

	ECSCore* pECS = ECSCore::GetInstance();

	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PlaySessionState::OnPacketReceived);

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
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
	//				.ShapeType		= EShapeType::SIMULATION,
	//				.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
	//				.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
	//			};

	//			PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	//			StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticCollisionSphere(collisionCreateInfo);
	//			pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	//		}
	//	}
	//}

	//Preload some resources
	{
		TArray<GUID_Lambda> animations;
		const uint32 robotGUID			= ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		TArray<GUID_Lambda> running		= ResourceManager::LoadAnimationsFromFile("Robot/Running.fbx");
		TArray<GUID_Lambda> thriller	= ResourceManager::LoadAnimationsFromFile("Robot/Thriller.fbx");		
		TArray<GUID_Lambda> reload		= ResourceManager::LoadAnimationsFromFile("Robot/Reloading.fbx");

		MaterialProperties materialProperties;
		materialProperties.Albedo		= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		materialProperties.Roughness	= 1.0f;
		materialProperties.Metallic		= 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID		= robotGUID;
		robotMeshComp.MaterialGUID	= robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.pGraph	= DBG_NEW AnimationGraph(DBG_NEW AnimationState("thriller", thriller[0]));
		robotAnimationComp.Pose		= ResourceManager::GetMesh(robotGUID)->pSkeleton; // TODO: Safer way than getting the raw pointer (GUID for skeletons?)

		glm::vec3 position = glm::vec3(0.0f, 5.75f, -2.5f);
		glm::vec3 scale(1.0f);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(0.0f, 0.8f, 0.0f);

		robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("walking", animations[0], 2.0f));

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(-3.5f, 0.75f, 0.0f);
		robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("running", running[0]));

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(3.5f, 0.75f, 0.0f);

		AnimationState* pReloadState = DBG_NEW AnimationState("reload");
		ClipNode* pReload = pReloadState->CreateClipNode(reload[0], 2.0, true);
		pReload->AddTrigger(ClipTrigger(0.8, [](const ClipNode& clip, AnimationGraph& graph)
		{
			LOG_INFO("Trigger at 0.1 | RunningTime=%.4f | NormalizedTime=%.4f", clip.GetRunningTime(), clip.GetNormalizedTime());
			graph.TransitionToState("running");
		}));

		ClipNode*	pRunning	= pReloadState->CreateClipNode(running[0]);
		ClipNode*	pDancing	= pReloadState->CreateClipNode(animations[0]);
		BlendNode*	pBlendNode0	= pReloadState->CreateBlendNode(pDancing, pReload,		BlendInfo(1.0f, "mixamorig:Neck"));
		BlendNode*	pBlendNode1	= pReloadState->CreateBlendNode(pBlendNode0, pRunning,	BlendInfo(1.0f, "mixamorig:Spine"));
		pReloadState->SetOutputNode(pBlendNode1);

		AnimationGraph* pAnimationGraph = DBG_NEW AnimationGraph();
		pAnimationGraph->AddState(DBG_NEW AnimationState("running", running[0]));
		pAnimationGraph->AddState(pReloadState);
		pAnimationGraph->AddTransition(DBG_NEW Transition("running", "reload", 0.2));
		pAnimationGraph->AddTransition(DBG_NEW Transition("reload", "running"));
		robotAnimationComp.pGraph = pAnimationGraph;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		// Audio
		GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("halo_theme.wav");
		ISoundInstance3D* pSoundInstance = DBG_NEW SoundInstance3DFMOD(AudioAPI::GetDevice());
		const SoundInstance3DDesc desc =
		{
				.pName			= "RobotSoundInstance",
				.pSoundEffect	= ResourceManager::GetSoundEffect(soundGUID),
				.Flags			= FSoundModeFlags::SOUND_MODE_NONE,
				.Position		= position,
				.Volume			= 0.03f
		};

		pSoundInstance->Init(&desc);
		pECS->AddComponent<AudibleComponent>(entity, { pSoundInstance });
	}

	ClientSystem::GetInstance().Connect(m_pIPAddress);
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
		glm::vec3 forward = decoder.ReadVec3();
		uint32 teamIndex = decoder.ReadUInt32();

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

		const CameraDesc cameraDesc =
		{
			.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV"),
			.Width		= (float)window->GetWidth(),
			.Height		= (float)window->GetHeight(),
			.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane"),
			.FarPlane	= EngineConfig::GetFloatProperty("CameraFarPlane")
		};

		TArray<GUID_Lambda> animations;
		const uint32 robotGUID			= ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx", animations);
		bool animationsExist			= !animations.IsEmpty();

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
		if (animationsExist)
		{
			robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("dancing", animations[0]));
		}

		CreatePlayerDesc createPlayerDesc =
		{
			.IsLocal			= isLocal,
			.NetworkUID			= networkUID,
			.pClient			= event.pClient,
			.Position			= position,
			.Forward			= forward,
			.Scale				= glm::vec3(1.0f),
			.TeamIndex			= teamIndex,
			.pCameraDesc		= &cameraDesc,
			.MeshGUID			= robotGUID,
			.AnimationComponent = robotAnimationComp,
		};

		m_pLevel->CreateObject(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, &createPlayerDesc);

		return true;
	}

	return false;
}

void PlaySessionState::Tick(LambdaEngine::Timestamp delta)
{
	
}

void PlaySessionState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_HUDSystem.FixedTick(delta);
}
