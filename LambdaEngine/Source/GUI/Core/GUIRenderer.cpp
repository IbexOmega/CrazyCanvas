#include "GUI/Core/GUIRenderer.h"
#include "GUI/Core/GUIHelpers.h"
#include "GUI/Core/GUIRenderTarget.h"
#include "GUI/Core/GUITexture.h"
#include "GUI/Core/GUIShaderManager.h"
#include "GUI/Core/GUIPipelineStateCache.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/Sampler.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/StagingBufferCache.h"

#include "Memory/API/Malloc.h"

#include "Debug/Profiler.h"

#include "NoesisPCH.h"

#include "Engine/EngineLoop.h"

//#define PRINT_FUNC

namespace LambdaEngine
{
	GUIRenderer::GUIRenderer()
	{
	}

	GUIRenderer::~GUIRenderer()
	{
		if (m_View != nullptr)
		{
			m_View.Reset();
		}

		SAFERELEASE(m_pIndexBuffer);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pGUISampler);

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			SAFERELEASE(m_ppUtilityCommandLists[b]);
			SAFERELEASE(m_ppUtilityCommandAllocators[b]);
			SAFERELEASE(m_ppRenderCommandLists[b]);
			SAFERELEASE(m_ppRenderCommandAllocators[b]);

			for (DeviceChild* pGraphicsResource : m_pGraphicsResourcesToRemove[b])
			{
				SAFERELEASE(pGraphicsResource);
			}

			m_pGraphicsResourcesToRemove[b].Clear();

			for (Buffer* pParamsBuffer : m_pUsedParamsBuffers[b])
			{
				SAFERELEASE(pParamsBuffer);
			}

			m_pUsedParamsBuffers[b].Clear();

			for (DescriptorSet* pDescriptorSet : m_pUsedDescriptorSets[b])
			{
				SAFERELEASE(pDescriptorSet);
			}

			m_pUsedDescriptorSets[b].Clear();
		}

		SAFERELEASE(m_pDescriptorHeap);

		for (Buffer* pParamsBuffer : m_AvailableParamsBuffers)
		{
			SAFERELEASE(pParamsBuffer);
		}

		m_AvailableParamsBuffers.Clear();

		for (DescriptorSet* pDescriptorSet : m_AvailableDescriptorSets)
		{
			SAFERELEASE(pDescriptorSet);
		}

		m_AvailableDescriptorSets.Clear();

		SAFERELEASE(m_pMainRenderPass);

		if (!GUIPipelineStateCache::Release())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to release GUIPipelineStateCache");
		}
	}

	bool GUIRenderer::Init()
	{
		if (!CreateCommandLists())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Render Command lists");
			return false;
		}

		return true;
	}

	bool GUIRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		if (m_Initialized) return true;

		VALIDATE(pPreInitDesc != nullptr);
		VALIDATE(pPreInitDesc->BackBufferCount == BACK_BUFFER_COUNT);

		if (!CreateSampler())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Sampler");
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Descriptor Heap");
			return false;
		}
		
		if (!CreateRenderPass(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!GUIPipelineStateCache::Init(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[GUIRenderer]: Failed to initialize GUIPipelineStateCache");
			return false;
		}

		m_Initialized = true;
		return true;
	}
	
	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CreateRenderTarget(const char* pLabel, uint32_t width, uint32_t height, uint32_t sampleCount)
	{
#ifdef PRINT_FUNC
		LOG_INFO("GUIRenderer::CreateRenderTarget");
#endif

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

		Noesis::Ptr<Noesis::RenderTarget> renderTarget = *pRenderTarget;
		return renderTarget;
	}

	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CloneRenderTarget(const char* pLabel, Noesis::RenderTarget* pSurface)
	{
#ifdef PRINT_FUNC
		LOG_INFO("GUIRenderer::CloneRenderTarget");
#endif

		const GUIRenderTarget* pOriginal = reinterpret_cast<const GUIRenderTarget*>(pSurface);

		GUIRenderTarget* pRenderTarget = new GUIRenderTarget();
		if (!pRenderTarget->Init(pOriginal->GetDesc()))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Render Target");
			SAFEDELETE(pRenderTarget);
			return nullptr;
		}

		Noesis::Ptr<Noesis::RenderTarget> renderTarget = *pRenderTarget;
		return renderTarget;
	}

	Noesis::Ptr<Noesis::Texture> GUIRenderer::CreateTexture(const char* pLabel, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** ppData)
	{
#ifdef PRINT_FUNC
		LOG_INFO("GUIRenderer::CreateTexture");
#endif

		GUITextureDesc textureDesc = {};
		textureDesc.DebugName		= pLabel;
		textureDesc.Width			= width;
		textureDesc.Height			= height;
		textureDesc.MipLevelCount	= numLevels;
		textureDesc.Format			= NoesisFormatToLambdaFormat(format);
		textureDesc.ppData			= ppData;

		GUITexture* pTexture = new GUITexture();

		if (!pTexture->Init(BeginOrGetUtilityCommandList(), &textureDesc))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Texture");
			SAFEDELETE(pTexture);
			return nullptr;
		}

		Noesis::Ptr<Noesis::Texture> texture = *pTexture;
		return texture;
	}

	void GUIRenderer::UpdateTexture(Noesis::Texture* pTexture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* pData)
	{
		EndCurrentRenderPass();

#ifdef PRINT_FUNC
		LOG_INFO("UpdateTexture");
#endif

		CommandList*	pCommandList	= BeginOrGetUtilityCommandList();
		GUITexture*		pGUITexture		= reinterpret_cast<GUITexture*>(pTexture);
		pGUITexture->UpdateTexture(pCommandList, level, x, y, width, height, pData, ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
	}

	void GUIRenderer::BeginRender(bool offscreen)
	{
		UNREFERENCED_VARIABLE(offscreen);

		if (!m_IsInRenderPass)
		{
			CommandList* pCommandList = BeginOrGetRenderCommandList();
			if (!offscreen)
			{
				BeginMainRenderPass(pCommandList);
#ifdef PRINT_FUNC
				LOG_INFO("BeginRender");
#endif
			}
			else
			{
				BeginTileRenderPass(pCommandList);
#ifdef PRINT_FUNC
				LOG_INFO("BeginRender[Offscreen]");
#endif
			}
		}
	}

	void GUIRenderer::SetRenderTarget(Noesis::RenderTarget* pSurface)
	{
		VALIDATE(pSurface != nullptr);
		
		m_pCurrentRenderTarget = reinterpret_cast<GUIRenderTarget*>(pSurface);
		EndCurrentRenderPass();

#ifdef PRINT_FUNC
		LOG_INFO("SetRenderTarget W: %u, H: %u", m_pCurrentRenderTarget->GetDesc()->Width, m_pCurrentRenderTarget->GetDesc()->Height);
#endif
	}

	void GUIRenderer::BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight)
	{
		UNREFERENCED_VARIABLE(surfaceWidth);
		UNREFERENCED_VARIABLE(surfaceHeight);
		
		CommandList* pCommandList = BeginOrGetRenderCommandList();
		
		// Do not set to false here will check in end tile if we were in a renderpass
		EndCurrentRenderPass();

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

#ifdef PRINT_FUNC
		LOG_INFO("BeginTile W: %u, H: %u", surfaceWidth, surfaceHeight);
#endif
	}

	void GUIRenderer::EndTile()
	{
		EndCurrentRenderPass();
		m_TileBegun					= false;
#ifdef PRINT_FUNC
		LOG_INFO("EndTile");
#endif

		ResumeRenderPass();
	}

	void GUIRenderer::ResolveRenderTarget(Noesis::RenderTarget* pSurface, const Noesis::Tile* pTiles, uint32_t numTiles)
	{
		UNREFERENCED_VARIABLE(pSurface);
		UNREFERENCED_VARIABLE(pTiles);
		UNREFERENCED_VARIABLE(pSurface);

#ifdef PRINT_FUNC
		LOG_INFO("ResolveRenderTarget");
#endif
	}

	void GUIRenderer::EndRender()
	{
		EndCurrentRenderPass();
		m_RenderPassBegun = false;

		CommandList* pCommandList = BeginOrGetRenderCommandList();
		pCommandList->End();

#ifdef PRINT_FUNC
		LOG_INFO("EndRender");
#endif
	}

	void* GUIRenderer::MapVertices(uint32_t bytes)
	{
#ifdef PRINT_FUNC
		LOG_INFO("MapVertices");
#endif
		m_RequiredVertexBufferSize = uint64(bytes);
		m_pVertexStagingBuffer = StagingBufferCache::RequestBuffer(m_RequiredVertexBufferSize);
		return m_pVertexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapVertices()
	{
#ifdef PRINT_FUNC
		LOG_INFO("UnmapVertices");
#endif
		m_pVertexStagingBuffer->Unmap();

		CommandList* pCommandList = BeginOrGetRenderCommandList();
		EndCurrentRenderPass();

		//Update Vertex Buffer
		{
			if (m_pVertexBuffer == nullptr || m_pVertexBuffer->GetDesc().SizeInBytes < m_RequiredVertexBufferSize)
			{
				if (m_pVertexBuffer != nullptr) m_pGraphicsResourcesToRemove[m_ModFrameIndex].PushBack(m_pVertexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "GUI Vertex Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_VERTEX_BUFFER;
				bufferDesc.SizeInBytes	= m_RequiredVertexBufferSize;

				m_pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(m_pVertexStagingBuffer, 0, m_pVertexBuffer, 0, m_RequiredVertexBufferSize);
		}

		ResumeRenderPass();
	}

	void* GUIRenderer::MapIndices(uint32_t bytes)
	{
#ifdef PRINT_FUNC
		LOG_INFO("MapIndices");
#endif

		m_RequiredIndexBufferSize = uint64(bytes);
		m_pIndexStagingBuffer = StagingBufferCache::RequestBuffer(m_RequiredIndexBufferSize);
		return m_pIndexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapIndices()
	{
#ifdef PRINT_FUNC
		LOG_INFO("UnmapIndices");
#endif
		m_pIndexStagingBuffer->Unmap();

		CommandList* pCommandList = BeginOrGetRenderCommandList();
		EndCurrentRenderPass();

		//Update Index Buffer
		{
			if (m_pIndexBuffer == nullptr || m_pIndexBuffer->GetDesc().SizeInBytes < m_RequiredIndexBufferSize)
			{
				if (m_pIndexBuffer != nullptr) m_pGraphicsResourcesToRemove[m_ModFrameIndex].PushBack(m_pIndexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "GUI Index Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
				bufferDesc.SizeInBytes	= m_RequiredIndexBufferSize;

				m_pIndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(m_pIndexStagingBuffer, 0, m_pIndexBuffer, 0, m_RequiredIndexBufferSize);
			pCommandList->BindIndexBuffer(m_pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT16);
		}

		ResumeRenderPass();
	}

	void GUIRenderer::DrawBatch(const Noesis::Batch& batch)
	{
		const TextureView* pBackBuffer = m_pBackBuffers[m_BackBufferIndex].Get();
		CommandList* pRenderCommandList = BeginOrGetRenderCommandList();

		NoesisShaderData shaderData = NoesisGetShaderData(batch.shader.v);

		Buffer* pParamsBuffer;

		//Update GUI Params
		{
			GUIParamsData paramsData = {};

			if (batch.projMtx != nullptr)
			{
				paramsData.ProjMatrix = glm::mat4(
					(*batch.projMtx)[0],	(*batch.projMtx)[4],	(*batch.projMtx)[8],	(*batch.projMtx)[12],
					(*batch.projMtx)[1],	(*batch.projMtx)[5],	(*batch.projMtx)[9],	(*batch.projMtx)[13],
					(*batch.projMtx)[2],	(*batch.projMtx)[6],	(*batch.projMtx)[10],	(*batch.projMtx)[14],
					(*batch.projMtx)[3],	(*batch.projMtx)[7],	(*batch.projMtx)[11],	(*batch.projMtx)[15]);
			}

			GUITexture* pTextTexture = nullptr;

			if (batch.glyphs != nullptr)	
				pTextTexture = reinterpret_cast<GUITexture*>(batch.glyphs);
			else if (batch.image != nullptr)	
				pTextTexture = reinterpret_cast<GUITexture*>(batch.image);

			if (pTextTexture != nullptr)
			{
				paramsData.TextSize.x		= float32(pTextTexture->GetWidth());
				paramsData.TextSize.y		= float32(pTextTexture->GetHeight());
				paramsData.TextPixelSize.x	= 1.0f / paramsData.TextSize.x;
				paramsData.TextPixelSize.y	= 1.0f / paramsData.TextSize.y;
			}

			if (batch.rgba != nullptr)
			{
				paramsData.RGBA.r = (*batch.rgba)[0];
				paramsData.RGBA.g = (*batch.rgba)[1];
				paramsData.RGBA.b = (*batch.rgba)[2];
				paramsData.RGBA.a = (*batch.rgba)[3];
			}
		
			if (batch.opacity != nullptr)
				paramsData.Opacity = *batch.opacity;

			if (batch.radialGrad != nullptr)
			{
				paramsData.RadialGrad0.x = (*batch.radialGrad)[0];
				paramsData.RadialGrad0.y = (*batch.radialGrad)[1];
				paramsData.RadialGrad0.z = (*batch.radialGrad)[2];
				paramsData.RadialGrad0.w = (*batch.radialGrad)[3];
				paramsData.RadialGrad1.x = (*batch.radialGrad)[4];
				paramsData.RadialGrad1.y = (*batch.radialGrad)[5];
				paramsData.RadialGrad1.z = (*batch.radialGrad)[6];
				paramsData.RadialGrad1.w = (*batch.radialGrad)[7];
			}
			
			if (batch.effectParams != nullptr)
			{
				memcpy(paramsData.pEffectParams, batch.effectParams, sizeof(paramsData.pEffectParams));
			}

			Buffer* pParamsStagingBuffer = StagingBufferCache::RequestBuffer(sizeof(GUIParamsData));
			void* pMapped = pParamsStagingBuffer->Map();
			memcpy(pMapped, &paramsData, sizeof(GUIParamsData));
			pParamsStagingBuffer->Unmap();

			pParamsBuffer = CreateOrGetParamsBuffer();
			CommandList* pUtilityCommandList = BeginOrGetUtilityCommandList();
			pUtilityCommandList->CopyBuffer(pParamsStagingBuffer, 0, pParamsBuffer, 0, sizeof(GUIParamsData));
		}

		//Write to Descriptor Set
		{
			uint64 paramsOffset		= 0;
			uint64 paramsSize		= sizeof(GUIParamsData);

			DescriptorSet* pDescriptorSet = CreateOrGetDescriptorSet();
			pDescriptorSet->WriteBufferDescriptors(&pParamsBuffer, &paramsOffset, &paramsSize, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
			if (batch.pattern != nullptr) 
				pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.pattern)->GetTextureViewToBind(), &m_pGUISampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			if (batch.ramps != nullptr)	
				pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.ramps)->GetTextureViewToBind(), &m_pGUISampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			
			if (batch.image != nullptr)
			{
				// Cannot explain why we need this part, but we get no validation errors now
				GUITexture* pTexture = reinterpret_cast<GUITexture*>(batch.image);
				{
					EndCurrentRenderPass();

					pRenderCommandList->TransitionBarrier(
						pTexture->GetTexture(),
						PIPELINE_STAGE_FLAG_TOP,
						PIPELINE_STAGE_FLAG_PIXEL_SHADER,
						0,
						FMemoryAccessFlag::MEMORY_ACCESS_FLAG_SHADER_READ,
						ETextureState::TEXTURE_STATE_DONT_CARE,
						ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
				}

				pDescriptorSet->WriteTextureDescriptors(pTexture->GetTextureViewToBind(), &m_pGUISampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}

			if (batch.glyphs != nullptr)	
				pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.glyphs)->GetTextureViewToBind(), &m_pGUISampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 4, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			if (batch.shadow != nullptr)	
				pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.shadow)->GetTextureViewToBind(), &m_pGUISampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 5, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);

			pRenderCommandList->BindDescriptorSetGraphics(pDescriptorSet, GUIPipelineStateCache::GetPipelineLayout(), 0);
		}

		uint32 width	= pBackBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height	= pBackBuffer->GetDesc().pTexture->GetDesc().Height;

		//Bind Pipeline State & Dynamic State
		{
			PipelineState* pPipelineState = GUIPipelineStateCache::GetPipelineState(
				batch.shader.v,
				batch.renderState.f.stencilMode,
				batch.renderState.f.colorEnable != 0,
				batch.renderState.f.blendMode	!= 0,
				m_TileBegun,
				shaderData);

			Viewport viewport = { };
			viewport.MinDepth	= 0.0f;
			viewport.MaxDepth	= 1.0f;
			viewport.Width		= float32(width);
			viewport.Height		= float32(height);
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;

			ScissorRect scissorRect = { };
			scissorRect.Width	= width;
			scissorRect.Height	= height;
			scissorRect.x		= 0;
			scissorRect.y		= 0;

			pRenderCommandList->SetViewports(&viewport, 0, 1);
			pRenderCommandList->SetScissorRects(&scissorRect, 0, 1);
			pRenderCommandList->SetStencilTestReference(EStencilFace::STENCIL_FACE_FRONT_AND_BACK, batch.stencilRef);

			pRenderCommandList->BindGraphicsPipeline(pPipelineState);
		}

		//Draw
		{
			uint64 vertexByteOffset = uint64(batch.vertexOffset);

			pRenderCommandList->BindVertexBuffers(&m_pVertexBuffer, 0, &vertexByteOffset, 1);

			ResumeRenderPass();

#ifdef PRINT_FUNC
			LOG_INFO("Draw");
#endif
			// Might not be the most effective way to abort, but it works
			if (m_IsInRenderPass)
				pRenderCommandList->DrawIndexInstanced(batch.numIndices, 1, batch.startIndex, 0, 0);
		}
	}

	void GUIRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		TArray<DeviceChild*>& resourcesToRemove = m_pGraphicsResourcesToRemove[m_ModFrameIndex];

		if (!resourcesToRemove.IsEmpty())
		{
			for (DeviceChild* pGraphicsResource : resourcesToRemove)
			{
				SAFERELEASE(pGraphicsResource);
			}

			resourcesToRemove.Clear();
		}

		//Make Param Buffers available again
		{
			TArray<Buffer*>& buffersNowAvailable = m_pUsedParamsBuffers[m_ModFrameIndex];

			for (Buffer* pParamsBuffer : buffersNowAvailable)
			{
				m_AvailableParamsBuffers.PushBack(pParamsBuffer);
			}

			buffersNowAvailable.Clear();
		}

		//Make Descriptor Sets available again
		{
			TArray<DescriptorSet*>& descriptorsNowAvailable = m_pUsedDescriptorSets[m_ModFrameIndex];

			if (!descriptorsNowAvailable.IsEmpty())
			{
				for (DescriptorSet* pDescriptorSet : descriptorsNowAvailable)
				{
					m_AvailableDescriptorSets.PushBack(pDescriptorSet);
				}

				descriptorsNowAvailable.Clear();
			}
		}
	}

	void GUIRenderer::UpdateTextureResource(
		const String& resourceName,
		const TextureView* const* ppPerImageTextureViews,
		const TextureView* const* ppPerSubImageTextureViews,
		const Sampler* const* ppPerImageSamplers,
		uint32 imageCount,
		uint32 subImageCount,
		bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			VALIDATE(imageCount == BACK_BUFFER_COUNT);

			for (uint32 i = 0; i < imageCount; i++)
			{
				m_pBackBuffers[i] = MakeSharedRef(ppPerSubImageTextureViews[i]);
			}
		}
		else if (resourceName == "NOESIS_GUI_DEPTH_STENCIL" && subImageCount == 1)
		{
			m_DepthStencilTextureView = MakeSharedRef(ppPerSubImageTextureViews[0]);
		}
	}

	void GUIRenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
#ifdef PRINT_FUNC
		LOG_INFO("Render");
#endif
		m_ModFrameIndex		= modFrameIndex;
		m_BackBufferIndex	= backBufferIndex;

		if (!sleeping)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_pGraphicsResourcesToRemove[m_ModFrameIndex];

			if (!resourcesToRemove.IsEmpty())
			{
				for (DeviceChild* pGraphicsResource : resourcesToRemove)
				{
					SAFERELEASE(pGraphicsResource);
				}

				resourcesToRemove.Clear();
			}

			//Make Param Buffers available again
			{
				TArray<Buffer*>& buffersNowAvailable = m_pUsedParamsBuffers[m_ModFrameIndex];

				for (Buffer* pParamsBuffer : buffersNowAvailable)
				{
					m_AvailableParamsBuffers.PushBack(pParamsBuffer);
				}

				buffersNowAvailable.Clear();
			}

			//Make Descriptor Sets available again
			{
				TArray<DescriptorSet*>& descriptorsNowAvailable = m_pUsedDescriptorSets[m_ModFrameIndex];

				if (!descriptorsNowAvailable.IsEmpty())
				{
					for (DescriptorSet* pDescriptorSet : descriptorsNowAvailable)
					{
						m_AvailableDescriptorSets.PushBack(pDescriptorSet);
					}

					descriptorsNowAvailable.Clear();
				}
			}
		}

		if (m_View.GetPtr() == nullptr)
			return;

		//Todo: Use UpdateRenderTree return value
		m_View->Update(EngineLoop::GetTimeSinceStart().AsSeconds());
		m_View->GetRenderer()->UpdateRenderTree();

		if (!sleeping)
		{
			m_View->GetRenderer()->RenderOffscreen();
			m_View->GetRenderer()->Render();

			CommandList* pUtilityCommandList = m_ppUtilityCommandLists[m_ModFrameIndex];
			CommandList* pRenderCommandList = m_ppRenderCommandLists[m_ModFrameIndex];

			if (pUtilityCommandList->IsRecording())
			{
				pUtilityCommandList->End();
				(*ppFirstExecutionStage)		= pUtilityCommandList;
				(*ppSecondaryExecutionStage)	= pRenderCommandList;
			}
			else
			{
				(*ppFirstExecutionStage) = pRenderCommandList;
			}
		}
	}

	void GUIRenderer::SetView(Noesis::Ptr<Noesis::IView> view)
	{
		m_View.Reset();
		m_View = view;
		if (m_View != nullptr)
		{
			m_View->GetRenderer()->Init(this);
		}
	}

	CommandList* GUIRenderer::BeginOrGetUtilityCommandList()
	{
		CommandList* pCommandList = m_ppUtilityCommandLists[m_ModFrameIndex];

		if (!pCommandList->IsRecording())
		{
			SecondaryCommandListBeginDesc beginDesc = {};

			m_ppUtilityCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	CommandList* GUIRenderer::BeginOrGetRenderCommandList()
	{
		CommandList* pCommandList = m_ppRenderCommandLists[m_ModFrameIndex];

		if (!pCommandList->IsRecording())
		{
			SecondaryCommandListBeginDesc beginDesc = {};

			m_ppRenderCommandAllocators[m_ModFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
		}

		return pCommandList;
	}

	void GUIRenderer::BeginMainRenderPass(CommandList* pCommandList)
	{
		//Begin RenderPass
		if (!m_RenderPassBegun)
		{
			const TextureView* pBackBuffer = m_pBackBuffers[m_BackBufferIndex].Get();

			BeginRenderPassDesc beginRenderPassDesc = {};
			beginRenderPassDesc.pRenderPass			= m_pMainRenderPass;
			beginRenderPassDesc.ppRenderTargets		= &pBackBuffer;
			beginRenderPassDesc.RenderTargetCount	= 1;
			beginRenderPassDesc.pDepthStencil		= m_DepthStencilTextureView.Get();
			beginRenderPassDesc.Width				= pBackBuffer->GetDesc().pTexture->GetDesc().Width;
			beginRenderPassDesc.Height				= pBackBuffer->GetDesc().pTexture->GetDesc().Height;
			beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
			beginRenderPassDesc.pClearColors		= m_pMainRenderPassClearColors;
			beginRenderPassDesc.ClearColorCount		= 2;
			beginRenderPassDesc.Offset.x			= 0;
			beginRenderPassDesc.Offset.y			= 0;

			pCommandList->BeginRenderPass(&beginRenderPassDesc);
			m_RenderPassBegun	= true;
			m_IsInRenderPass	= true;
		}
	}

	void GUIRenderer::BeginTileRenderPass(CommandList* pCommandList)
	{
		//Begin RenderPass
		if (!m_TileBegun)
		{
			if (m_pCurrentRenderTarget && m_pCurrentRenderTarget->GetRenderPass())
			{
				BeginRenderPassDesc beginRenderPass = {};
				beginRenderPass.pRenderPass			= m_pCurrentRenderTarget->GetRenderPass();
				beginRenderPass.ppRenderTargets		= m_pCurrentRenderTarget->GetRenderTargets();
				beginRenderPass.RenderTargetCount	= 2; // The rendertarget + resolve target
				beginRenderPass.pDepthStencil		= m_pCurrentRenderTarget->GetDepthStencil();
				beginRenderPass.Width				= m_pCurrentRenderTarget->GetWidth();
				beginRenderPass.Height				= m_pCurrentRenderTarget->GetHeight();
				beginRenderPass.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
				beginRenderPass.pClearColors		= m_pCurrentRenderTarget->GetClearColors();
				beginRenderPass.ClearColorCount		= m_pCurrentRenderTarget->GetClearColorCount();
				beginRenderPass.Offset.x			= 0;
				beginRenderPass.Offset.y			= 0;

				pCommandList->BeginRenderPass(&beginRenderPass);
				m_IsInRenderPass = true;
			}
			
			m_TileBegun = true;
		}
	}

	Buffer* GUIRenderer::CreateOrGetParamsBuffer()
	{
		Buffer* pParamsBuffer;

		if (!m_AvailableParamsBuffers.IsEmpty())
		{
			pParamsBuffer = m_AvailableParamsBuffers.GetBack();
			m_AvailableParamsBuffers.PopBack();
			m_pUsedParamsBuffers[m_ModFrameIndex].PushBack(pParamsBuffer);
		}
		else
		{
			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "GUI Params Buffer";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
			bufferDesc.SizeInBytes	= sizeof(GUIParamsData);

			pParamsBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);

			m_pUsedParamsBuffers[m_ModFrameIndex].PushBack(pParamsBuffer);
		}

		return pParamsBuffer;
	}

	DescriptorSet* GUIRenderer::CreateOrGetDescriptorSet()
	{
		DescriptorSet* pDescriptorSet;

		if (!m_AvailableDescriptorSets.IsEmpty())
		{
			pDescriptorSet = m_AvailableDescriptorSets.GetBack();
			m_AvailableDescriptorSets.PopBack();
			m_pUsedDescriptorSets[m_ModFrameIndex].PushBack(pDescriptorSet);
		}
		else
		{
			pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("GUI Renderer Descriptor Set", GUIPipelineStateCache::GetPipelineLayout(), 0, m_pDescriptorHeap);
			m_pUsedDescriptorSets[m_ModFrameIndex].PushBack(pDescriptorSet);
		}

		return pDescriptorSet;
	}

	void GUIRenderer::ResumeRenderPass()
	{
#ifdef PRINT_FUNC
		LOG_INFO("Trying to resume renderpass: tileBegun: %d", m_TileBegun);
#endif

		if (!m_IsInRenderPass)
		{
			CommandList* pCommandList = BeginOrGetRenderCommandList();
			if (m_RenderPassBegun)
			{
				m_RenderPassBegun = false;
				BeginMainRenderPass(pCommandList);
#ifdef PRINT_FUNC
				LOG_INFO("Resuming Main");
#endif
			}

			if (m_TileBegun)
			{
				m_TileBegun = false;
				BeginTileRenderPass(pCommandList);

#ifdef PRINT_FUNC
				LOG_INFO("Resuming Tile");
#endif
			}
		}
	}

	void GUIRenderer::EndCurrentRenderPass()
	{
		// If we are currently rendering we exit the current renderpass
		if (m_IsInRenderPass)
		{
			CommandList* pCommandList = BeginOrGetRenderCommandList();
			pCommandList->EndRenderPass();

			m_IsInRenderPass = false;
#ifdef PRINT_FUNC
			LOG_INFO("Ending RenderPass");
#endif
		}
	}

	bool GUIRenderer::CreateCommandLists()
	{
		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			//Buffer Copy Command Lists
			{
				m_ppUtilityCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("GUI Utility Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

				if (!m_ppUtilityCommandAllocators[b])
				{
					return false;
				}

				CommandListDesc commandListDesc = {};
				commandListDesc.DebugName			= "GUI Utility List " + std::to_string(b);
				commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppUtilityCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppUtilityCommandAllocators[b], &commandListDesc);

				if (!m_ppUtilityCommandLists[b])
				{
					return false;
				}
			}

			//Render Command Lists
			{
				m_ppRenderCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("GUI Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

				if (!m_ppRenderCommandAllocators[b])
				{
					return false;
				}

				CommandListDesc commandListDesc = {};
				commandListDesc.DebugName			= "GUI Render Command List " + std::to_string(b);
				commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				m_ppRenderCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

				if (!m_ppRenderCommandLists[b])
				{
					return false;
				}

				//CommandList* pCommandList = m_ppRenderCommandLists[b];

				//Profiler::GetGPUProfiler()->AddTimestamp(pCommandList, "GUI Render Command List");

				//pCommandList->Begin(nullptr);
				//Profiler::GetGPUProfiler()->ResetTimestamp(pCommandList);
				//pCommandList->End();
				//RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
				//RenderAPI::GetGraphicsQueue()->Flush();
			}
		}

		return true;
	}
	bool GUIRenderer::CreateDescriptorHeap()
	{
		constexpr uint32 DESCRIPTOR_COUNT = 16;
		constexpr uint32 DESCRIPTOR_SET_COUNT = 1024;

		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureDescriptorCount						= DESCRIPTOR_COUNT;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.ConstantBufferDescriptorCount				= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount		= DESCRIPTOR_COUNT;
		descriptorCountDesc.AccelerationStructureDescriptorCount		= DESCRIPTOR_COUNT;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "GUI Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= DESCRIPTOR_SET_COUNT;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);

		return m_pDescriptorHeap != nullptr;
	}

	bool GUIRenderer::CreateSampler()
	{
		SamplerDesc samplerLinearDesc = {};
		samplerLinearDesc.DebugName				= "GUI Linear Sampler";
		samplerLinearDesc.MinFilter				= EFilterType::FILTER_TYPE_LINEAR;
		samplerLinearDesc.MagFilter				= EFilterType::FILTER_TYPE_LINEAR;
		samplerLinearDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_LINEAR;
		samplerLinearDesc.AddressModeU			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerLinearDesc.AddressModeV			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerLinearDesc.AddressModeW			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerLinearDesc.MipLODBias			= 0.0f;
		samplerLinearDesc.AnisotropyEnabled		= false;
		samplerLinearDesc.MaxAnisotropy			= 16;
		samplerLinearDesc.MinLOD				= 0.0f;
		samplerLinearDesc.MaxLOD				= 1.0f;

		m_pGUISampler = RenderAPI::GetDevice()->CreateSampler(&samplerLinearDesc);

		return m_pGUISampler != nullptr;
	}

	bool GUIRenderer::CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= pBackBufferAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState		= pBackBufferAttachmentDesc->FinalState;

		RenderPassAttachmentDesc depthStencilAttachmentDesc = {};
		depthStencilAttachmentDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthStencilAttachmentDesc.SampleCount		= 1;
		depthStencilAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_DONT_CARE;
		depthStencilAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_DONT_CARE;
		depthStencilAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_STORE;
		depthStencilAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		{
			depthStencilAttachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			depthStencilAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_LOAD;

			RenderPassDesc renderPassDesc = {};
			renderPassDesc.DebugName			= "GUI Render Pass";
			renderPassDesc.Attachments			= { colorAttachmentDesc, depthStencilAttachmentDesc };
			renderPassDesc.Subpasses			= { subpassDesc };
			renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

			m_pMainRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);
		}

		m_pMainRenderPassClearColors[0].Color[0]	= 0.0f;
		m_pMainRenderPassClearColors[0].Color[1]	= 0.0f;
		m_pMainRenderPassClearColors[0].Color[2]	= 0.0f;
		m_pMainRenderPassClearColors[0].Color[3]	= 0.0f;

		m_pMainRenderPassClearColors[1].Depth	= 1.0f;
		m_pMainRenderPassClearColors[1].Stencil	= 0;

		return true;
	}
}