#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Player/HealthSystem.h"
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
#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/PacketType.h"

PlaySessionState::PlaySessionState(LambdaEngine::IPAddress* pIPAddress) :
	m_pIPAddress(pIPAddress),
	m_MultiplayerClient()
{
}

PlaySessionState::~PlaySessionState()
{
}

void PlaySessionState::Init()
{
	using namespace LambdaEngine;

	ClientSystem::GetInstance();

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();

	m_WeaponSystem.Init();
	m_HealthSystem.Init();
	m_HUDSystem.Init();
	m_MultiplayerClient.InitInternal();

	ECSCore* pECS = ECSCore::GetInstance();

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[0]
		};
		Match::CreateMatch(&matchDescription);
	}

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

		glm::vec3 position = glm::vec3(0.0f, 0.75f, -2.5f);
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

void PlaySessionState::Tick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerClient.TickMainThreadInternal(delta);
}

void PlaySessionState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_HUDSystem.FixedTick(delta);
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
	m_HUDSystem.FixedTick(delta);
}