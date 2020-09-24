#include "GUI/GUIRenderer.h"
#include "GUI/GUIHelpers.h"
#include "GUI/GUIRenderTarget.h"
#include "GUI/GUITexture.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/StagingBufferCache.h"

#include "Memory/API/Malloc.h"

#include "Debug/Profiler.h"

namespace LambdaEngine
{
	GUIRenderer::GUIRenderer()
	{
	}

	GUIRenderer::~GUIRenderer()
	{
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppRenderCommandLists[b]);
			SAFERELEASE(m_ppRenderCommandAllocators[b]);

			for (Buffer* pBuffer : m_pBuffersToRemove[b])
			{
				SAFERELEASE(pBuffer);
			}

			m_pBuffersToRemove[b].Clear();
		}

		SAFEDELETE_ARRAY(m_ppRenderCommandLists);
		SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
		SAFEDELETE_ARRAY(m_pBuffersToRemove);
	}

	bool GUIRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc != nullptr);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateCommandLists())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Render Command lists");
			return false;
		}
		
		m_pBuffersToRemove = DBG_NEW TArray<Buffer*>[m_BackBufferCount];

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
		GUITextureDesc textureDesc = {};
		textureDesc.DebugName		= pLabel;
		textureDesc.Width			= width;
		textureDesc.Height			= height;
		textureDesc.MipLevelCount	= numLevels;
		textureDesc.Format			= NoesisFormatToLamdaFormat(format);
		textureDesc.ppData			= ppData;

		GUITexture* pTexture = new GUITexture();

		if (!pTexture->Init(BeginOrGetCommandList(), &textureDesc))
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
		CommandList* pCommandList = BeginOrGetCommandList();
		pCommandList->Begin(nullptr);
	}

	void GUIRenderer::SetRenderTarget(Noesis::RenderTarget* pSurface)
	{
		VALIDATE(pSurface != nullptr);
		
		m_pCurrentRenderTarget = reinterpret_cast<GUIRenderTarget*>(pSurface);
	}

	void GUIRenderer::BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight)
	{
		CommandList* pCommandList = BeginOrGetCommandList();

		Viewport viewport = { };
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= float(tile.width);
		viewport.Height		= float(tile.height);
		viewport.x			= float(tile.x);
		viewport.y			= float(tile.y);

		pCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = { };
		scissorRect.Width	= tile.width;
		scissorRect.Height	= tile.height;
		scissorRect.x		= tile.x;
		scissorRect.y		= tile.y;

		pCommandList->SetScissorRects(&scissorRect, 0, 1);

		BeginRenderPassDesc beginRenderPass = {};
		beginRenderPass.pRenderPass			= m_pCurrentRenderTarget->GetRenderPass();
		beginRenderPass.ppRenderTargets		= m_pCurrentRenderTarget->GetRenderTargets();
		beginRenderPass.RenderTargetCount	= 1;
		beginRenderPass.pDepthStencil		= m_pCurrentRenderTarget->GetDepthStencil();
		beginRenderPass.Width				= surfaceWidth;
		beginRenderPass.Height				= surfaceHeight;
		beginRenderPass.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPass.pClearColors		= m_pCurrentRenderTarget->GetClearColors();
		beginRenderPass.ClearColorCount		= m_pCurrentRenderTarget->GetClearColorCount();
		beginRenderPass.Offset.x			= 0.0f;
		beginRenderPass.Offset.y			= 0.0f;

		pCommandList->BeginRenderPass(&beginRenderPass);
	}

	void GUIRenderer::EndTile()
	{
		CommandList* pCommandList = BeginOrGetCommandList();
		pCommandList->EndRenderPass();
	}

	void GUIRenderer::ResolveRenderTarget(Noesis::RenderTarget* pSurface, const Noesis::Tile* pTiles, uint32_t numTiles)
	{
		UNREFERENCED_VARIABLE(pSurface);
		UNREFERENCED_VARIABLE(pTiles);
		UNREFERENCED_VARIABLE(pSurface);
	}

	void GUIRenderer::EndRender()
	{
		CommandList* pCommandList = BeginOrGetCommandList();
		pCommandList->End();
	}

	void* GUIRenderer::MapVertices(uint32_t bytes)
	{
		m_pVertexStagingBuffer = StagingBufferCache::RequestBuffer(uint64(bytes));
		return m_pVertexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapVertices()
	{
		m_pVertexStagingBuffer->Unmap();
	}

	void* GUIRenderer::MapIndices(uint32_t bytes)
	{
		m_pIndexStagingBuffer = StagingBufferCache::RequestBuffer(uint64(bytes));
		return m_pIndexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapIndices()
	{
		m_pIndexStagingBuffer->Unmap();
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

	void GUIRenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage)
	{
		m_ModFrameIndex = modFrameIndex;

		TArray<Buffer*>& frameBuffersToRemove = m_pBuffersToRemove[m_ModFrameIndex];

		if (!frameBuffersToRemove.IsEmpty())
		{
			for (Buffer* pBuffer : frameBuffersToRemove)
			{
				SAFERELEASE(pBuffer);
			}

			frameBuffersToRemove.Clear();
		}
	}

	CommandList* GUIRenderer::BeginOrGetCommandList()
	{
		CommandList* pCommandList = m_ppRenderCommandLists[m_ModFrameIndex];

		if (!pCommandList->IsBegin())
		{
			SecondaryCommandListBeginDesc beginDesc = {};

			m_ppRenderCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	bool GUIRenderer::CreateCommandLists()
	{
		m_ppRenderCommandAllocators	= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppRenderCommandLists		= DBG_NEW CommandList*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("GUI Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName			= "ImGui Render Command List " + std::to_string(b);
			commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppRenderCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

			if (!m_ppRenderCommandLists[b])
			{
				return false;
			}

			CommandList* pCommandList = m_ppRenderCommandLists[b];

			Profiler::GetGPUProfiler()->AddTimestamp(pCommandList, "GUI Render Command List");

			pCommandList->Begin(nullptr);
			Profiler::GetGPUProfiler()->ResetTimestamp(pCommandList);
			pCommandList->End();
			RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
			RenderAPI::GetGraphicsQueue()->Flush();
		}

		return true;
	}
}