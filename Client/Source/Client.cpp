#include "Client.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphDescriptionParser.h"

#include <imgui.h>

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkDebugger.h"

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

    m_pClient = ClientUDP::Create(this, 1024, 100);

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

void Client::OnConnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnConnectingUDP()");
}

void Client::OnConnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    using namespace LambdaEngine;

    LOG_MESSAGE("OnConnectedUDP()");

    for (int i = 0; i < 1; i++)
    {
        NetworkPacket* pPacket = m_pClient->GetFreePacket(1);
        BinaryEncoder encoder(pPacket);
        encoder.WriteInt32(i);
        m_pClient->SendReliable(pPacket, this);
    }
}

void Client::OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnectingUDP()");
}

void Client::OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnectedUDP()");
}

void Client::OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_VARIABLE(pClient);
    UNREFERENCED_VARIABLE(pPacket);
    LOG_MESSAGE("OnPacketReceivedUDP(%s)", pPacket->ToString().c_str());
}

void Client::OnServerFullUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_ERROR("OnServerFullUDP()");
}

void Client::OnPacketDelivered(LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketDelivered(%s)", pPacket->ToString().c_str());
}

void Client::OnPacketResent(LambdaEngine::NetworkPacket* pPacket, uint8 tries)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketResent(%d)", tries);
}

void Client::OnPacketMaxTriesReached(LambdaEngine::NetworkPacket* pPacket, uint8 tries)
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
        NetworkPacket* packet = m_pClient->GetFreePacket(packetType);
        BinaryEncoder encoder(packet);
        encoder.WriteString("Test Message");
        m_pClient->SendReliable(packet, this);
    }
}

void Client::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(delta);

	m_pRenderer->Begin(delta);

	ImGui::ShowDemoWindow();

	NetworkDebugger::RenderStatisticsWithImGUI(m_pClient);

	m_pRenderer->Render(delta);

	m_pRenderer->End(delta);
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

	GUID_Lambda fullscreenQuadShaderGUID = ResourceManager::LoadShaderFromFile("../Assets/Shaders/FullscreenQuad.glsl", FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::GLSL);
	GUID_Lambda shadingPixelShaderGUID = ResourceManager::LoadShaderFromFile("../Assets/Shaders/StaticPixel.glsl", FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::GLSL);

	std::vector<RenderStageDesc> renderStages;

	const char* pShadingRenderStageName = "Shading Render Stage";
	GraphicsManagedPipelineStateDesc			shadingPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	{
		shadingRenderStageAttachments.push_back({
			RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,
			EAttachmentType::OUTPUT_COLOR,
			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,
			BACK_BUFFER_COUNT, EFormat::FORMAT_B8G8R8A8_UNORM
		});

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName = "Shading Pass Push Constants";
		pushConstants.DataSize = sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName = pShadingRenderStageName;
		renderStage.pAttachments = shadingRenderStageAttachments.data();
		renderStage.AttachmentCount = (uint32)shadingRenderStageAttachments.size();

		shadingPipelineStateDesc.pName = "Shading Pass Pipeline State";
		shadingPipelineStateDesc.VertexShader = fullscreenQuadShaderGUID;
		shadingPipelineStateDesc.PixelShader = shadingPixelShaderGUID;

		renderStage.PipelineType = EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType = ERenderStageDrawType::FULLSCREEN_QUAD;
		renderStage.GraphicsPipeline.pIndexBufferName = nullptr;
		renderStage.GraphicsPipeline.pMeshIndexBufferName = nullptr;
		renderStage.GraphicsPipeline.pGraphicsDesc = &shadingPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pName = "Render Graph";
	renderGraphDesc.CreateDebugGraph = RENDERING_DEBUG_ENABLED;
	renderGraphDesc.pRenderStages = renderStages.data();
	renderGraphDesc.RenderStageCount = (uint32)renderStages.size();
	renderGraphDesc.BackBufferCount = BACK_BUFFER_COUNT;
	renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	renderGraphDesc.pScene = nullptr;

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	m_pRenderGraph->Init(renderGraphDesc);

	Window* pWindow = CommonApplication::Get()->GetMainWindow();
	uint32 renderWidth = pWindow->GetWidth();
	uint32 renderHeight = pWindow->GetHeight();

	{
		RenderStageParameters shadingRenderStageParameters = {};
		shadingRenderStageParameters.pRenderStageName = pShadingRenderStageName;
		shadingRenderStageParameters.Graphics.Width = renderWidth;
		shadingRenderStageParameters.Graphics.Height = renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	}

	m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	RendererDesc rendererDesc = {};
	rendererDesc.pName = "Renderer";
	rendererDesc.Debug = RENDERING_DEBUG_ENABLED;
	rendererDesc.pRenderGraph = m_pRenderGraph;
	rendererDesc.pWindow = CommonApplication::Get()->GetMainWindow();
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
		Client* pSandbox = DBG_NEW Client();
        
        return pSandbox;
    }
}
