#include "GUI/GUIRenderer.h"
#include "GUI/GUIHelpers.h"
#include "GUI/GUIRenderTarget.h"
#include "GUI/GUITexture.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"

#include "Memory/API/Malloc.h"

namespace LambdaEngine
{
	GUIRenderer::GUIRenderer()
	{
	}

	GUIRenderer::~GUIRenderer()
	{
		SAFERELEASE(m_pGUICommandList);
		SAFERELEASE(m_pGUICommandAllocator);
	}

	bool GUIRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		m_pGUICommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("GUI Renderer Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName			= "GUI Renderer Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_SECONDARY;
		commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_pGUICommandList = RenderAPI::GetDevice()->CreateCommandList(m_pGUICommandAllocator, &commandListDesc);

		return true;
	}
	
	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CreateRenderTarget(const char* pLabel, uint32_t width, uint32_t height, uint32_t sampleCount)
	{
		GUIRenderTargetDesc renderTargetDesc = {};
		renderTargetDesc.DebugName		= pLabel;
		renderTargetDesc.Width			= width;
		renderTargetDesc.Height			= height;
		renderTargetDesc.SampleCount	= sampleCount;

		GUIRenderTarget* pRenderTarget = new GUIRenderTarget();

		if (!pRenderTarget->Init(&renderTargetDesc))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Render Target");
			SAFEDELETE(pRenderTarget);
			return nullptr;
		}

		return Noesis::Ptr<Noesis::RenderTarget>(pRenderTarget);
	}

	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CloneRenderTarget(const char* pLabel, Noesis::RenderTarget* pSurface)
	{
		const GUIRenderTarget* pOriginal = reinterpret_cast<const GUIRenderTarget*>(pSurface);

		GUIRenderTarget* pRenderTarget = new GUIRenderTarget();

		if (!pRenderTarget->Init(pRenderTarget->GetDesc()))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Render Target");
			SAFEDELETE(pRenderTarget);
			return nullptr;
		}

		return Noesis::Ptr<Noesis::RenderTarget>(pRenderTarget);
	}

	Noesis::Ptr<Noesis::Texture> GUIRenderer::CreateTexture(const char* pLabel, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** ppData)
	{
		if (!m_pGUICommandList->IsBegin())
		{
			SecondaryCommandListBeginDesc beginDesc = {};

			m_pGUICommandAllocator->Reset();
			m_pGUICommandList->Begin();
		}

		GUITextureDesc textureDesc = {};
		textureDesc.DebugName		= pLabel;
		textureDesc.Width			= width;
		textureDesc.Height			= height;
		textureDesc.MipLevelCount	= numLevels;
		textureDesc.Format			= NoesisFormatToLamdaFormat(format);
		textureDesc.ppData			= ppData;

		GUITexture* pTexture = new GUITexture();

		if (!pTexture->Init(m_pGUICommandList, &textureDesc))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Texture");
			SAFEDELETE(pTexture);
			return nullptr;
		}

		return Noesis::Ptr<Noesis::Texture>(pTexture);
	}

	void GUIRenderer::UpdateTexture(Noesis::Texture* pTexture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* pData)
	{
		CommandList* pCommandList = RenderSystem::GetInstance().GetRenderGraph()->AcquireGraphicsCopyCommandList();

		GUITexture* pGUITexture = reinterpret_cast<GUITexture*>(pTexture);
		pGUITexture->UpdateTexture(pCommandList, level, x, y, width, height, pData, ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, m_ModFrameIndex);
	}

	void GUIRenderer::BeginRender(bool offscreen)
	{
		UNREFERENCED_VARIABLE(offscreen);
	}

	void GUIRenderer::SetRenderTarget(Noesis::RenderTarget* pSurface)
	{
		VALIDATE(pSurface != nullptr);
		
		m_pCurrentRenderTarget = reinterpret_cast<GUIRenderTarget*>(pSurface);
	}

	void GUIRenderer::BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight)
	{
		m_pCurrentRenderTarget->
	}

	void GUIRenderer::EndTile()
	{
	}

	void GUIRenderer::ResolveRenderTarget(Noesis::RenderTarget* pSurface, const Noesis::Tile* pTiles, uint32_t numTiles)
	{
	}

	void GUIRenderer::EndRender()
	{
	}

	void* GUIRenderer::MapVertices(uint32_t bytes)
	{
		return nullptr;
	}

	void GUIRenderer::UnmapVertices()
	{
	}

	void* GUIRenderer::MapIndices(uint32_t bytes)
	{
		return nullptr;
	}

	void GUIRenderer::UnmapIndices()
	{
	}

	void GUIRenderer::DrawBatch(const Noesis::Batch& batch)
	{
	}

	void GUIRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void GUIRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void GUIRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
	}

	void GUIRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
	}

	void GUIRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
	}

	void GUIRenderer::Render(CommandAllocator* pGraphicsCommandAllocator, CommandList* pGraphicsCommandList, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppPrimaryExecutionStage, CommandList** ppSecondaryExecutionStage)
	{
		m_ModFrameIndex = modFrameIndex;
	}
}