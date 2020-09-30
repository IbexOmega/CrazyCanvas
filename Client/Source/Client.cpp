#include "Client.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioSystem.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/NetworkDebugger.h"

#include "Game/StateManager.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Game/ECS/Systems/Networking/ClientSystem.h"

#include "ECS/ECSCore.h"

#include "Rendering/RenderGraph.h"

#include "NetworkingState.h"

#include <argh/argh.h>

using namespace LambdaEngine;

Client::Client()
{

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Client");
	PlatformConsole::SetTitle("Client Console");

	LoadRendererResources();

	StateManager::GetInstance()->EnqueueStateTransition(DBG_NEW NetworkingState(), STATE_TRANSITION::PUSH);

	m_MeshSphereGUID = ResourceManager::LoadMeshFromFile("sphere.obj");


	ClientSystem::GetInstance().Connect(IPAddress::Get("81.170.143.133"));

	//NetworkDiscovery::EnableClient("Crazy Canvas", this);
}

Client::~Client()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);
}

void Client::OnServerFound(const LambdaEngine::BinaryDecoder& decoder, const LambdaEngine::IPEndPoint& endPoint)
{
	UNREFERENCED_VARIABLE(decoder);
	LOG_MESSAGE("OnServerFound(%s)", endPoint.ToString().c_str());
}

bool Client::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

void Client::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Client::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

bool Client::LoadRendererResources()
{
	using namespace LambdaEngine;

	// For Skybox RenderGraph
	{
		String skybox[]
		{
			"Skybox/right.png",
			"Skybox/left.png",
			"Skybox/top.png",
			"Skybox/bottom.png",
			"Skybox/front.png",
			"Skybox/back.png"
		};

		GUID_Lambda cubemapTexID = ResourceManager::LoadCubeTexturesArrayFromFile("Cubemap Texture", skybox, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);

		Texture* pCubeTexture = ResourceManager::GetTexture(cubemapTexID);
		TextureView* pCubeTextureView = ResourceManager::GetTextureView(cubemapTexID);
		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	return true;
}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& parser)
	{
		UNREFERENCED_VARIABLE(parser);
		Client* pClient = DBG_NEW Client();
		return pClient;
	}
}