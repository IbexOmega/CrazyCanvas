#include "CrazyCanvas.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/EntityMaskManager.h"

#include "RenderStages/PlayerRenderer.h"
#include "RenderStages/MeshPaintUpdater.h"
#include "RenderStages/HealthCompute.h"
#include "States/BenchmarkState.h"
#include "States/MainMenuState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/ServerState.h"

#include "Networking/API/NetworkUtils.h"

#include "World/LevelManager.h"

#include "Teams/TeamHelper.h"

#include "Engine/EngineConfig.h"

#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Lobby/PlayerManagerClient.h"
#include "Lobby/PlayerManagerServer.h"

#include "Chat/ChatManager.h"

#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/GameOverGUI.h"
#include "GUI/HUDGUI.h"
#include "GUI/MainMenuGUI.h"
#include "GUI/Core/GUIApplication.h"

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

CrazyCanvas::CrazyCanvas(const argh::parser& flagParser)
{
	using namespace LambdaEngine;

	constexpr const char* pGameName = "Crazy Canvas";
	constexpr const char* pDefaultStateStr = "crazycanvas";
	State* pStartingState = nullptr;
	String stateStr;

	flagParser({ "--state" }, pDefaultStateStr) >> stateStr;

	if (stateStr == "crazycanvas" || stateStr == "sandbox" || stateStr == "benchmark")
	{
		ClientSystem::Init(pGameName);
	}
	else if (stateStr == "server")
	{
		ServerSystem::Init(pGameName);
	}

	if (!RegisterGUIComponents())
	{
		LOG_ERROR("Failed to Register GUI Components");
	}

	if (!BindComponentTypeMasks())
	{
		LOG_ERROR("Failed to bind Component Type Masks");
	}

	if (!LevelManager::Init())
	{
		LOG_ERROR("Level Manager Init Failed");
	}

	if (!TeamHelper::Init())
	{
		LOG_ERROR("Team Helper Init Failed");
	}

	PacketType::Init();
	PacketTranscoderSystem::GetInstance().Init();

	RenderSystem::GetInstance().AddCustomRenderer(DBG_NEW MeshPaintUpdater());

	if (stateStr == "server")
	{
		RenderSystem::GetInstance().AddCustomRenderer(DBG_NEW HealthCompute());
	}
	else
	{
		RenderSystem::GetInstance().AddCustomRenderer(DBG_NEW PlayerRenderer());
	}


	RenderSystem::GetInstance().InitRenderGraphs();

	LoadRendererResources();

	if (stateStr == "crazycanvas")
	{
		pStartingState = DBG_NEW MainMenuState();
	}
	else if (stateStr == "sandbox")
	{
		pStartingState = DBG_NEW SandboxState();
	}
	else if (stateStr == "server")
	{
		String clientHostIDStr;
		flagParser(1, "") >> clientHostIDStr;
		pStartingState = DBG_NEW ServerState(clientHostIDStr);
	}
	else if (stateStr == "benchmark")
	{
		pStartingState = DBG_NEW BenchmarkState();
	}

	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::PUSH);

	if(MultiplayerUtils::IsServer())
		PlayerManagerServer::Init();
	else
		PlayerManagerClient::Init();

	ChatManager::Init();
}

CrazyCanvas::~CrazyCanvas()
{
	if (!LevelManager::Release())
	{
		LOG_ERROR("Level Manager Release Failed");
	}

	ChatManager::Release();
	PlayerManagerBase::Release();
	PacketType::Release();
}

void CrazyCanvas::Tick(LambdaEngine::Timestamp delta)
{
	Render(delta);
}

void CrazyCanvas::FixedTick(LambdaEngine::Timestamp delta)
{
	if (LambdaEngine::MultiplayerUtils::IsServer())
		PlayerManagerServer::FixedTick(delta);
}

void CrazyCanvas::Render(LambdaEngine::Timestamp)
{}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		return DBG_NEW CrazyCanvas(flagParser);
	}
}

bool CrazyCanvas::RegisterGUIComponents()
{
	Noesis::RegisterComponent<CountdownGUI>();
	Noesis::RegisterComponent<GameOverGUI>();
	Noesis::RegisterComponent<DamageIndicatorGUI>();
	Noesis::RegisterComponent<EnemyHitIndicatorGUI>();
	Noesis::RegisterComponent<HUDGUI>();
	Noesis::RegisterComponent<MainMenuGUI>();

	return true;
}

bool CrazyCanvas::LoadRendererResources()
{
	using namespace LambdaEngine;

	// For Skybox RenderGraph
	{
		// Test Skybox
		GUID_Lambda cubemapTexID = ResourceManager::LoadTextureCubeFromPanormaFile(
			"Skybox/daytime.hdr",
			EFormat::FORMAT_R16G16B16A16_SFLOAT,
			768,
			false);

		Texture*		pCubeTexture		= ResourceManager::GetTexture(cubemapTexID);
		TextureView*	pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);
		Sampler*		pLinearSampler		= Sampler::GetLinearSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName										= "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures					= &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews				= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppPerSubImageTextureViews	= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers					= &pLinearSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	// For Mesh painting in RenderGraph
	{
		GUID_Lambda brushMaskID = ResourceManager::LoadTextureFromFile("MeshPainting/BrushMaskV3.png", EFormat::FORMAT_R8G8B8A8_UNORM, false, false);

		Texture* pTexture = ResourceManager::GetTexture(brushMaskID);
		TextureView* pTextureView = ResourceManager::GetTextureView(brushMaskID);
		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "BRUSH_MASK";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	return true;
}

bool CrazyCanvas::BindComponentTypeMasks()
{
	using namespace LambdaEngine;

	// NOTE: Previous implementation had a comment that said the bitmask was 0xF, even though
	// the value that is being set is 0x10. This seems to be assumed on other places but doesn't seem to cause
	// any notable errors, but might have to be looked at later.
	EntityMaskManager::BindTypeToExtensionDesc(WeaponLocalComponent::Type(), { 0 }, false, 0x10);	// Bit = 0x10

	// Used to calculate health on the server for players only
	EntityMaskManager::BindTypeToExtensionDesc(HealthComponent::Type(),	{ 0 }, false, 0x20);	// Bit = 0x20

	EntityMaskManager::Finalize();

	return true;
}
