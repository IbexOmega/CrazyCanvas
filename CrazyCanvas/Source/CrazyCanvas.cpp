#include "CrazyCanvas.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Resources/ResourceManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "States/BenchmarkState.h"
#include "States/MainMenuState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/ServerState.h"

#include "Networking/API/NetworkUtils.h"

#include "World/LevelManager.h"

#include "Teams/TeamHelper.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"



#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

CrazyCanvas::CrazyCanvas(const argh::parser& flagParser)
{
	using namespace LambdaEngine;

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

	if (!LevelManager::Init())
	{
		LOG_ERROR("Level Manager Init Failed");
	}

	if (!TeamHelper::Init())
	{
		LOG_ERROR("Team Helper Init Failed");
	}

	LoadRendererResources();

	constexpr const char* pGameName = "Crazy Canvas";
	constexpr const char* pDefaultStateStr = "crazycanvas";
	constexpr const char* pDefaultIsHostStr = "-1";
	State* pStartingState = nullptr;
	String stateStr;

	String clientHostIDStr; // Used on server To Identify Host(Client transmits HostID)
	String serverHostIDStr; // Used on Client To Identify Host(Server transmits HostID)

	flagParser({ "--state" }, pDefaultStateStr) >> stateStr;

	if (stateStr == "server")
	{
		flagParser(1, pDefaultIsHostStr) >> serverHostIDStr;

		flagParser(2, pDefaultIsHostStr) >> clientHostIDStr;
	}

	if (stateStr == "crazycanvas")
	{
		ClientSystem::Init(pGameName);
		pStartingState = DBG_NEW MainMenuState();
	}
	else if (stateStr == "sandbox")
	{
		ClientSystem::Init(pGameName);
		pStartingState = DBG_NEW SandboxState();
	}
	else if (stateStr == "client")
	{
		ClientSystem::Init(pGameName);
		pStartingState = DBG_NEW PlaySessionState(NetworkUtils::GetLocalAddress());
	}
	else if (stateStr == "server")
	{
		ServerSystem::Init(pGameName);
		pStartingState = DBG_NEW ServerState(serverHostIDStr, clientHostIDStr);
	}
	else if (stateStr == "benchmark")
	{
		ClientSystem::Init(pGameName);
		pStartingState = DBG_NEW BenchmarkState();
	}

	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::PUSH);
}

CrazyCanvas::~CrazyCanvas()
{
	if (!LevelManager::Release())
	{
		LOG_ERROR("Level Manager Release Failed");
	}
}

void CrazyCanvas::Tick(LambdaEngine::Timestamp delta)
{
	Render(delta);
}

void CrazyCanvas::FixedTick(LambdaEngine::Timestamp)
{}

void CrazyCanvas::Render(LambdaEngine::Timestamp)
{}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		return DBG_NEW CrazyCanvas(flagParser);
	}
}

bool CrazyCanvas::LoadRendererResources()
{
	using namespace LambdaEngine;

	{
		String blueNoiseLUTFileNames[NUM_BLUE_NOISE_LUTS];

		for (uint32 i = 0; i < NUM_BLUE_NOISE_LUTS; i++)
		{
			char str[5];
			snprintf(str, 5, "%04d", i);
			blueNoiseLUTFileNames[i] = "LUTs/BlueNoise/256_256/HDR_RGBA_" + std::string(str) + ".png";
		}

		GUID_Lambda blueNoiseID = ResourceManager::LoadTextureArrayFromFile("Blue Noise Texture", blueNoiseLUTFileNames, NUM_BLUE_NOISE_LUTS, EFormat::FORMAT_R16_UNORM, false, false);

		Texture* pBlueNoiseTexture = ResourceManager::GetTexture(blueNoiseID);
		TextureView* pBlueNoiseTextureView = ResourceManager::GetTextureView(blueNoiseID);

		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc blueNoiseUpdateDesc = {};
		blueNoiseUpdateDesc.ResourceName = "BLUE_NOISE_LUT";
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures = &pBlueNoiseTexture;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pBlueNoiseTextureView;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&blueNoiseUpdateDesc);
	}

	// For Skybox RenderGraph
	{
		String skybox[]
		{
			"Skybox/px.png",
			"Skybox/nx.png",
			"Skybox/py.png",
			"Skybox/ny.png",
			"Skybox/pz.png",
			"Skybox/nz.png"
		};

		GUID_Lambda cubemapTexID = ResourceManager::LoadCubeTexturesArrayFromFile("Cubemap Texture", skybox, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false, false);

		Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
		TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);
		Sampler* pNearestSampler		= Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName							= "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	// For Mesh painting in RenderGraph
	{
		GUID_Lambda brushMaskID = ResourceManager::LoadTextureFromFile("MeshPainting/BrushMaskV2.png", EFormat::FORMAT_R8G8B8A8_UNORM, false, false);

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
