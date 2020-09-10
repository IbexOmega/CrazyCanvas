#include "Client.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/RenderGraph.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Audio/AudioSystem.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkDebugger.h"

#include "Networking/API/TCP/ClientTCP.h"

#ifdef LAMBDA_PLATFORM_MACOS
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 8;
#else
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 256;
#endif
constexpr const uint32 BACK_BUFFER_COUNT = 3;
constexpr const bool RENDERING_DEBUG_ENABLED = true;

Client::Client() :
    m_pClient(nullptr)
{
	using namespace LambdaEngine;

    CommonApplication::Get()->AddEventHandler(this);
    CommonApplication::Get()->GetMainWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

	InitRendererForEmpty();


    ClientDesc desc = {};
    desc.PoolSize               = 512;
    desc.MaxRetries             = 10;
    desc.ResendRTTMultiplier    = 2.0F;
    desc.Handler                = this;
    desc.Protocol               = EProtocol::UDP;

    m_pClient = NetworkUtils::CreateClient(desc);

    if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
    {
        LOG_ERROR("Failed to connect!");
    }
}

Client::~Client()
{
    m_pClient->Release();

	SAFEDELETE(m_pRenderGraph);
	SAFEDELETE(m_pRenderer);
}

void Client::OnConnecting(LambdaEngine::IClient* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnConnecting()");
}

void Client::OnConnected(LambdaEngine::IClient* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    using namespace LambdaEngine;

    LOG_MESSAGE("OnConnected()");

    /*for (int i = 0; i < 1; i++)
    {
        NetworkPacket* pPacket = m_pClient->GetFreePacket(1);
        BinaryEncoder encoder(pPacket);
        encoder.WriteInt32(i);
        m_pClient->SendReliable(pPacket, this);
    }*/


	NetworkSegment* pPacket = m_pClient->GetFreePacket(1);
	BinaryEncoder encoder(pPacket);
	encoder.WriteString("Christoffer");
	m_pClient->SendReliable(pPacket, this);
}

void Client::OnDisconnecting(LambdaEngine::IClient* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnecting()");
}

void Client::OnDisconnected(LambdaEngine::IClient* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnected()");
}

void Client::OnPacketReceived(LambdaEngine::IClient* pClient, LambdaEngine::NetworkSegment* pPacket)
{
    UNREFERENCED_VARIABLE(pClient);
    UNREFERENCED_VARIABLE(pPacket);
    LOG_MESSAGE("OnPacketReceived(%s)", pPacket->ToString().c_str());
}

void Client::OnServerFull(LambdaEngine::IClient* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_ERROR("OnServerFull()");
}

void Client::OnPacketDelivered(LambdaEngine::NetworkSegment* pPacket)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketDelivered(%s)", pPacket->ToString().c_str());
}

void Client::OnPacketResent(LambdaEngine::NetworkSegment* pPacket, uint8 tries)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketResent(%d)", tries);
}

void Client::OnPacketMaxTriesReached(LambdaEngine::NetworkSegment* pPacket, uint8 tries)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_ERROR("OnPacketMaxTriesReached(%d)", tries);
}

void Client::OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(modifierMask);
	UNREFERENCED_VARIABLE(isRepeat);

    if (key == EKey::KEY_ENTER)
    {
        if (m_pClient->IsConnected())
            m_pClient->Disconnect();
        else
            m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
    }
    else
    {
        uint16 packetType = 0;
        NetworkSegment* packet = m_pClient->GetFreePacket(packetType);
        BinaryEncoder encoder(packet);
        encoder.WriteString("Test Message");
        m_pClient->SendReliable(packet, this);
    }
}

void Client::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(delta);

    m_pRenderGraph->Update();

    m_pRenderer->NewFrame(delta);

    m_pRenderGraphEditor->RenderGUI();

	ImGui::ShowDemoWindow();

	NetworkDebugger::RenderStatisticsWithImGUI(m_pClient);

    m_pRenderer->PrepareRender(delta);

    m_pRenderer->Render();
}

void Client::FixedTick(LambdaEngine::Timestamp delta)
{
    using namespace LambdaEngine;
    UNREFERENCED_VARIABLE(delta);

    //m_pClient->Flush();
}

bool Client::InitRendererForEmpty()
{
	using namespace LambdaEngine;


    m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

    SceneDesc sceneDesc = { };
    sceneDesc.Name = "Test Scene";
    sceneDesc.RayTracingEnabled = false;
    m_pScene->Init(sceneDesc);


    m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();

    RenderGraphStructureDesc renderGraphStructure = m_pRenderGraphEditor->CreateRenderGraphStructure("", true);

    RenderGraphDesc renderGraphDesc = {};
    renderGraphDesc.pRenderGraphStructureDesc = &renderGraphStructure;
    renderGraphDesc.BackBufferCount = BACK_BUFFER_COUNT;
    renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
    renderGraphDesc.pScene = m_pScene;

    m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());
    m_pRenderGraph->Init(&renderGraphDesc);



    m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

    RendererDesc rendererDesc = {};
    rendererDesc.Name = "Renderer";
    rendererDesc.Debug = RENDERING_DEBUG_ENABLED;
    rendererDesc.pRenderGraph = m_pRenderGraph;
    rendererDesc.pWindow = CommonApplication::Get()->GetMainWindow().Get();
    rendererDesc.BackBufferCount = BACK_BUFFER_COUNT;

    m_pRenderer->Init(&rendererDesc);

    if (RENDERING_DEBUG_ENABLED)
    {
        ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());
    }
    return true;
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Client* pClient = DBG_NEW Client();
        
        return pClient;
    }
}
