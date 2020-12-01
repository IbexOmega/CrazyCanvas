#include "CrazyCanvas.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Resources/ResourceManager.h"
#include "Resources/ResourceCatalog.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/EntityMaskManager.h"

#include "RenderStages/PlayerRenderer.h"
#include "RenderStages/MeshPaintUpdater.h"
#include "RenderStages/HealthCompute.h"
#include "RenderStages/FirstPersonWeaponRenderer.h"
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
#include "GUI/EscapeMenuGUI.h"
#include "GUI/PromptGUI.h"
#include "GUI/KillFeedGUI.h"
#include "GUI/ScoreBoardGUI.h"
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

	const String& protocol = EngineConfig::GetStringProperty(CONFIG_OPTION_NETWORK_PROTOCOL);

	if (stateStr == "crazycanvas" || stateStr == "sandbox" || stateStr == "benchmark")
	{
		ClientSystemDesc desc = {};
		desc.Name					= pGameName;
		desc.PoolSize				= 8196;
		desc.MaxRetries				= 10;
		desc.ResendRTTMultiplier	= 3.0f;
		desc.Protocol				= EProtocolParser::FromString(protocol);
		desc.PingInterval			= Timestamp::Seconds(1);
		desc.PingTimeout			= Timestamp::Seconds(5);
		desc.UsePingSystem			= EngineConfig::GetBoolProperty(CONFIG_OPTION_NETWORK_PING_SYSTEM);

		ClientSystem::Init(desc);
	}
	else if (stateStr == "server")
	{
		ServerSystemDesc desc = {};
		desc.Name					= pGameName;
		desc.PoolSize				= 8196;
		desc.MaxRetries				= 10;
		desc.ResendRTTMultiplier	= 3.0f;
		desc.Protocol				= EProtocolParser::FromString(protocol);
		desc.PingInterval			= Timestamp::Seconds(1);
		desc.PingTimeout			= Timestamp::Seconds(5);
		desc.UsePingSystem			= EngineConfig::GetBoolProperty(CONFIG_OPTION_NETWORK_PING_SYSTEM);
		desc.MaxClients				= 10;

		ServerSystem::Init(desc);
	}

	if (!ResourceCatalog::Init())
	{
		LOG_ERROR("Failed to Load Resource Catalog Resources");
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
		RenderSystem::GetInstance().AddCustomRenderer(DBG_NEW FirstPersonWeaponRenderer());
	}

	RenderSystem::GetInstance().InitRenderGraphs();

	InitRendererResources();

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

	m_MeshPaintHandler.Release();
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
	Noesis::RegisterComponent<EscapeMenuGUI>();
	Noesis::RegisterComponent<DamageIndicatorGUI>();
	Noesis::RegisterComponent<PromptGUI>();
	Noesis::RegisterComponent<EnemyHitIndicatorGUI>();
	Noesis::RegisterComponent<HUDGUI>();
	Noesis::RegisterComponent<KillFeedGUI>();
	Noesis::RegisterComponent<ScoreBoardGUI>();
	Noesis::RegisterComponent<MainMenuGUI>();

	return true;
}

bool CrazyCanvas::InitRendererResources()
{
	using namespace LambdaEngine;

	// For Skybox RenderGraph
	{
		// Test Skybox
		GUID_Lambda cubemapTexID = ResourceManager::LoadTextureCubeFromPanormaFile(
			"Skybox/daytime.hdr",
			EFormat::FORMAT_R16G16B16A16_SFLOAT,
			512,
			true);

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
		Texture* pTexture = ResourceManager::GetTexture(ResourceCatalog::BRUSH_MASK_GUID);
		TextureView* pTextureView = ResourceManager::GetTextureView(ResourceCatalog::BRUSH_MASK_GUID);
		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "BRUSH_MASK";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	m_MeshPaintHandler.Init();

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
