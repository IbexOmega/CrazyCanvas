#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Rendering/EntityMaskManager.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include "Lobby/PlayerManagerClient.h"

PlaySessionState::PlaySessionState(bool singlePlayer) :
	m_Singleplayer(singlePlayer),
	m_MultiplayerClient()
{
	using namespace LambdaEngine;

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Init();
	}
}

PlaySessionState::~PlaySessionState()
{
	using namespace LambdaEngine;

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Release();
	}
}

void PlaySessionState::Init()
{
	using namespace LambdaEngine;

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", false);

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[0]
		};
		Match::CreateMatch(&matchDescription);
	}

	m_HUDSystem.Init();

	CommonApplication::Get()->SetMouseVisibility(false);

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Setup();
	}
	else
	{
		//Called to tell the server we are ready to start the match
		PlayerManagerClient::SetLocalPlayerStateLoading();
	}
}

void PlaySessionState::Tick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerClient.TickMainThreadInternal(delta);
}

void PlaySessionState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_HUDSystem.FixedTick(delta);
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
}
