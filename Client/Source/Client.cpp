#include "Client.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Resources/ResourceManager.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/NetworkDebugger.h"

#include "Game/StateManager.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Rendering/RenderGraph.h"

#include "NetworkingState.h"

#include <argh/argh.h>

#define TYPE_ADD_ENTITY 1

using namespace LambdaEngine;

Client::Client() :
	m_pClient(nullptr),
	m_IsBenchmarking(false),
	m_BenchmarkPackets(0)
{

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Client");
	PlatformConsole::SetTitle("Client Console");

	LoadRendererResources();

	StateManager::GetInstance()->EnqueueStateTransition(DBG_NEW NetworkingState(), STATE_TRANSITION::PUSH);

	m_MeshSphereGUID = ResourceManager::LoadMeshFromFile("sphere.obj");


    ClientDesc desc = {};
    desc.PoolSize               = 1024;
    desc.MaxRetries             = 10;
    desc.ResendRTTMultiplier    = 2.0F;
    desc.Handler                = this;
    desc.Protocol               = EProtocol::UDP;
	desc.PingInterval			= Timestamp::Seconds(1);
	desc.PingTimeout			= Timestamp::Seconds(3);
	desc.UsePingSystem			= true;

	m_pClient = NetworkUtils::CreateClient(desc);

	if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
	{
		LOG_ERROR("Failed to connect!");
	}

	//NetworkDiscovery::EnableClient("Crazy Canvas", this);
}

Client::~Client()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);

	if(m_pClient)
		m_pClient->Release();
}

void Client::OnServerFound(const LambdaEngine::BinaryDecoder& decoder, const LambdaEngine::IPEndPoint& endPoint)
{
	LOG_MESSAGE("OnServerFound(%s)", endPoint.ToString().c_str());
}

void Client::OnConnecting(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnecting()");
}

void Client::OnConnected(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnected()");
}

void Client::OnDisconnecting(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnecting()");
}

void Client::OnDisconnected(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnected()");
}

void Client::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pClient);
	UNREFERENCED_VARIABLE(pPacket);
	LOG_MESSAGE("OnPacketReceived(%s)", pPacket->ToString().c_str());

	if (pPacket->GetType() == TYPE_ADD_ENTITY)
	{
		uint32 reliableUID = pPacket->GetReliableUID();
		BinaryDecoder decoder(pPacket);
		bool isMyEntity = decoder.ReadBool();
		glm::vec3 pos = decoder.ReadVec3();
		glm::vec3 color = decoder.ReadVec3();

		
		Job addEntityJob;
		addEntityJob.Components =
		{
			{ RW, PositionComponent::Type() } ,
			{ RW, RotationComponent::Type() } ,
			{ RW, ScaleComponent::Type() } ,
			{ RW, MeshComponent::Type() } ,
			{ RW, NetworkComponent::Type() }
		};
		addEntityJob.Function = [isMyEntity, pos, color, reliableUID, this]
		{
			ECSCore* pECS = ECSCore::GetInstance();

			MaterialProperties materialProperties = {};
			materialProperties.Roughness = 0.1f;
			materialProperties.Metallic = 0.0f;
			materialProperties.Albedo = glm::vec4(color, 1.0f);

			MeshComponent meshComponent;
			meshComponent.MeshGUID = m_MeshSphereGUID;
			meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
				"Mirror Material " + std::to_string(reliableUID),
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				materialProperties);

			Entity entity = pECS->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity,	{ pos, true });
			pECS->AddComponent<RotationComponent>(entity,	{ glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity,		{ glm::vec3(1.0f), true });
			pECS->AddComponent<MeshComponent>(entity,		meshComponent);
			pECS->AddComponent<NetworkComponent>(entity,	{});
		};
		
		ECSCore::GetInstance()->ScheduleJobASAP(addEntityJob);
	}
}

void Client::OnServerFull(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_ERROR("OnServerFull()");
}

void Client::OnClientReleased(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_ERROR("OnClientReleased()");
}

void Client::OnPacketDelivered(NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_INFO("OnPacketDelivered(%s)", pPacket->ToString().c_str());
}

void Client::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_INFO("OnPacketResent(%d)", tries);
}

void Client::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_ERROR("OnPacketMaxTriesReached(%d)", tries);
}

bool Client::OnKeyPressed(const KeyPressedEvent& event)
{

	if (InputActionSystem::IsActive("PLAYER_FORWARD"))
	{
		LOG_INFO("PLAYER_FORWARD ACTIVE");
	}
	else if (InputActionSystem::IsActive("PLAYER_BACKWARD"))
	{
		LOG_INFO("PLAYER_BACKWARD ACTIVE");
	}
	else if (InputActionSystem::IsActive("PLAYER_LEFT"))
	{
		LOG_INFO("PLAYER_LEFT ACTIVE");
	}
	else if (InputActionSystem::IsActive("PLAYER_RIGHT"))
	{
		LOG_INFO("PLAYER_RIGHT ACTIVE");
	}
	else if (InputActionSystem::IsActive("CHANGE_KEYBINDING"))
	{
		LOG_INFO("CHANGE_KEYBINDING ACTIVE");
		InputActionSystem::ChangeKeyBinding("PLAYER_FORWARD", EKey::KEY_UP);
	}


	if (event.Key == EKey::KEY_ENTER)
	{
		if (m_pClient->IsConnected())
			m_pClient->Disconnect("User Requested");
		else
			m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.1.65"), 4444));
	}
	else if(event.Key == EKey::KEY_HOME)
	{
		m_IsBenchmarking = true;
	}

	return false;
}

void Client::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	if (m_pClient)
	{
		if (m_pClient->IsConnected() && m_IsBenchmarking)
		{
			RunningBenchMark();
		}
		NetworkDebugger::RenderStatistics(m_pClient);
	}
}

void Client::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

}

void Client::RunningBenchMark()
{
	if (m_BenchmarkPackets++ < 100000)
	{
		NetworkSegment* pPacket = m_pClient->GetFreePacket(420);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt32(m_BenchmarkPackets);
		m_pClient->SendReliable(pPacket, this);
	}
	else
	{
		m_IsBenchmarking = false;
		m_BenchmarkPackets = 0;
	}
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
