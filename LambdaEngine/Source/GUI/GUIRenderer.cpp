#include "GUI/GUIRenderer.h"
#include "GUI/GUIHelpers.h"
#include "GUI/GUIRenderTarget.h"
#include "GUI/GUITexture.h"
#include "GUI/GUIShaderManager.h"
#include "GUI/GUIPipelineStateCache.h"

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

namespace LambdaEngine
{
	GUIRenderer::GUIRenderer()
	{
	}

	GUIRenderer::~GUIRenderer()
	{
		SAFERELEASE(m_pParamsBuffer);

		for (GUITexture* pTexture : m_GUITextures)
		{
			SAFEDELETE(pTexture);
		}

		for (GUIRenderTarget* pRenderTarget : m_GUIRenderTargets)
		{
			SAFEDELETE(pRenderTarget);
		}

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			SAFERELEASE(m_ppRenderCommandLists[b]);
			SAFERELEASE(m_ppRenderCommandAllocators[b]);

			for (Buffer* pBuffer : m_pBuffersToRemove[b])
			{
				SAFERELEASE(pBuffer);
			}

			m_pBuffersToRemove[b].Clear();

			for (DescriptorSet* pDescriptorSet : m_pUsedDescriptorSets[b])
			{
				SAFERELEASE(pDescriptorSet);
			}

			m_pUsedDescriptorSets[b].Clear();
		}

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
		VALIDATE(pPreInitDesc != nullptr);
		VALIDATE(pPreInitDesc->BackBufferCount == BACK_BUFFER_COUNT);

		if (!CreateBuffers())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Buffers");
			return false;
		}

		if (!CreateDescriptorHeap())
		{
			LOG_ERROR("[GUIRenderer]: Failed to create Descriptor Heap");
			return false;
		}
		
		if (!CreateRenderPass(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!GUIPipelineStateCache::Init(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to initialize GUIPipelineStateCache");
			return false;
		}

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

		m_GUIRenderTargets.PushBack(pRenderTarget);
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
		textureDesc.Format			= NoesisFormatToLambdaFormat(format);
		textureDesc.ppData			= ppData;

		GUITexture* pTexture = new GUITexture();

		if (!pTexture->Init(BeginOrGetCommandList(), &textureDesc))
		{
			LOG_ERROR("[GUIRenderer]: Failed to create GUI Texture");
			SAFEDELETE(pTexture);
			return nullptr;
		}

		m_GUITextures.PushBack(pTexture);
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
		m_RequiredVertexBufferSize = uint64(bytes);
		m_pVertexStagingBuffer = StagingBufferCache::RequestBuffer(m_RequiredVertexBufferSize);
		return m_pVertexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapVertices()
	{
		m_pVertexStagingBuffer->Unmap();

		//Update Vertex Buffer
		{
			CommandList* pCommandList = BeginOrGetCommandList();

			if (m_pVertexBuffer == nullptr || m_pVertexBuffer->GetDesc().SizeInBytes < m_RequiredVertexBufferSize)
			{
				if (m_pVertexBuffer != nullptr) m_pBuffersToRemove->PushBack(m_pVertexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "GUI Vertex Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes	= m_RequiredVertexBufferSize;

				m_pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(m_pVertexStagingBuffer, 0, m_pVertexBuffer, 0, m_RequiredVertexBufferSize);
		}
	}

	void* GUIRenderer::MapIndices(uint32_t bytes)
	{
		m_RequiredIndexBufferSize = uint64(bytes);
		m_pIndexStagingBuffer = StagingBufferCache::RequestBuffer(m_RequiredIndexBufferSize);
		return m_pIndexStagingBuffer->Map();
	}

	void GUIRenderer::UnmapIndices()
	{
		m_pIndexStagingBuffer->Unmap();

		//Update Index Buffer
		{
			CommandList* pCommandList = BeginOrGetCommandList();

			if (m_pIndexBuffer == nullptr || m_pIndexBuffer->GetDesc().SizeInBytes < m_RequiredIndexBufferSize)
			{
				if (m_pIndexBuffer != nullptr) m_pBuffersToRemove->PushBack(m_pIndexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "GUI Index Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
				bufferDesc.SizeInBytes	= m_RequiredIndexBufferSize;

				m_pIndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(m_pIndexStagingBuffer, 0, m_pIndexBuffer, 0, m_RequiredIndexBufferSize);
		}
	}

	void GUIRenderer::DrawBatch(const Noesis::Batch& batch)
	{
		CommandList* pCommandList = BeginOrGetCommandList();

		NoesisShaderData shaderData = NoesisGetShaderData(batch.shader.v);

		//Update GUI Params
		{
			GUIParamsData paramsData = {};

			if (batch.projMtx != nullptr)
			{
				paramsData.ProjMatrix = glm::mat4(
					(*batch.projMtx)[0],	(*batch.projMtx)[1],	(*batch.projMtx)[2],	(*batch.projMtx)[3],
					(*batch.projMtx)[4],	(*batch.projMtx)[5],	(*batch.projMtx)[6],	(*batch.projMtx)[7],
					(*batch.projMtx)[8],	(*batch.projMtx)[9],	(*batch.projMtx)[10],	(*batch.projMtx)[11],
					(*batch.projMtx)[12],	(*batch.projMtx)[13],	(*batch.projMtx)[14],	(*batch.projMtx)[15]);
			}

			GUITexture* pTextTexture = nullptr;

			if		(batch.glyphs	!= nullptr)	pTextTexture = reinterpret_cast<GUITexture*>(batch.glyphs);
			else if (batch.image	!= nullptr)	pTextTexture = reinterpret_cast<GUITexture*>(batch.image);

			if (pTextTexture != nullptr)
			{
				paramsData.TextSize.x		= pTextTexture->GetWidth();
				paramsData.TextSize.x		= pTextTexture->GetHeight();
				paramsData.TextPixelSize.x	= 1.0f / float32(pTextTexture->GetWidth());
				paramsData.TextPixelSize.y	= 1.0f / float32(pTextTexture->GetHeight());
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
		
			memcpy(paramsData.pEffectParams, batch.effectParams, sizeof(paramsData.pEffectParams));

			Buffer* pParamsStagingBuffer = StagingBufferCache::RequestBuffer(sizeof(GUIParamsData));
			void* pMapped = pParamsStagingBuffer->Map();
			memcpy(pMapped, &paramsData, sizeof(GUIParamsData));
			pParamsStagingBuffer->Unmap();

			pCommandList->CopyBuffer(pParamsStagingBuffer, 0, m_pParamsBuffer, 0, sizeof(GUIParamsData));
		}

		//Write to Descriptor Set
		{
			uint64 vertexByteOffset = uint64(batch.vertexOffset	* shaderData.VertexSize);
			uint64 vertexByteSize	= uint64(batch.numVertices	* shaderData.VertexSize);

			uint64 paramsOffset		= 0;
			uint64 paramsSize		= sizeof(GUIParamsData);

			DescriptorSet* pDescriptorSet = CreateOrGetDescriptorSet();
			pDescriptorSet->WriteBufferDescriptors(&m_pVertexBuffer, &vertexByteOffset, &vertexByteSize,	0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			pDescriptorSet->WriteBufferDescriptors(&m_pParamsBuffer, &paramsOffset,		&paramsSize,		1, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
			if (batch.pattern != 0) pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.pattern)->GetTextureViewToBind(),	Sampler::GetLinearSamplerToBind(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
			if (batch.ramps != 0)	pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.ramps)->GetTextureViewToBind(),		Sampler::GetLinearSamplerToBind(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
			if (batch.image != 0)	pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.image)->GetTextureViewToBind(),		Sampler::GetLinearSamplerToBind(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 4, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
			if (batch.glyphs != 0)	pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.glyphs)->GetTextureViewToBind(),	Sampler::GetLinearSamplerToBind(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 5, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
			if (batch.shadow != 0)	pDescriptorSet->WriteTextureDescriptors(reinterpret_cast<GUITexture*>(batch.shadow)->GetTextureViewToBind(),	Sampler::GetLinearSamplerToBind(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 6, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);

			pCommandList->BindDescriptorSetGraphics(pDescriptorSet, GUIPipelineStateCache::GetPipelineLayout(), 0);
		}
		
		const TextureView* pBackBuffer = m_pBackBuffers[m_BackBufferIndex].Get();
		uint32 width	= pBackBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height	= pBackBuffer->GetDesc().pTexture->GetDesc().Height;

		//Bind Pipeline State & Dynamic State
		{
			PipelineState* pPipelineState = GUIPipelineStateCache::GetPipelineState(
				batch.shader.v,
				batch.renderState.f.colorEnable != 0,
				batch.renderState.f.blendMode	!= 0,
				shaderData);

			Viewport viewport = { };
			viewport.MinDepth	= 0.0f;
			viewport.MaxDepth	= 1.0f;
			viewport.Width		= float32(width);
			viewport.Height		= float32(height);
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;

			pCommandList->SetViewports(&viewport, 0, 1);

			ScissorRect scissorRect = { };
			scissorRect.Width	= width;
			scissorRect.Height	= height;
			scissorRect.x		= 0;
			scissorRect.y		= 0;

			pCommandList->SetScissorRects(&scissorRect, 0, 1);

			pCommandList->SetStencilTestEnabled(batch.renderState.f.stencilMode != Noesis::StencilMode::Disabled);
			pCommandList->SetStencilTestOp(
				EStencilFace::STENCIL_FACE_FRONT_AND_BACK, 
				EStencilOp::STENCIL_OP_KEEP,
				NoesisStencilModeToLambdaStencilOp(batch.renderState.f.stencilMode),
				EStencilOp::STENCIL_OP_KEEP,
				ECompareOp::COMPARE_OP_EQUAL);
			pCommandList->SetStencilTestReference(EStencilFace::STENCIL_FACE_FRONT_AND_BACK, batch.stencilRef);

			pCommandList->BindGraphicsPipeline(pPipelineState);
		}

		//Begin RenderPass
		{
			BeginRenderPassDesc beginRenderPassDesc = {};
			beginRenderPassDesc.pRenderPass			= m_pMainRenderPass;
			beginRenderPassDesc.ppRenderTargets		= &pBackBuffer;
			beginRenderPassDesc.RenderTargetCount	= 1;
			beginRenderPassDesc.pDepthStencil		= nullptr;
			beginRenderPassDesc.Width				= width;
			beginRenderPassDesc.Height				= height;
			beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
			beginRenderPassDesc.pClearColors		= nullptr;
			beginRenderPassDesc.ClearColorCount		= 0;
			beginRenderPassDesc.Offset.x			= 0;
			beginRenderPassDesc.Offset.y			= 0;

			pCommandList->BeginRenderPass(&beginRenderPassDesc);
		}

		//Draw
		{
			pCommandList->BindIndexBuffer(m_pIndexBuffer, batch.startIndex * sizeof(uint16), EIndexType::INDEX_TYPE_UINT16);
			pCommandList->DrawIndexInstanced(batch.numIndices, 1, batch.numIndices, 0, 0);
		}

		//End RenderPass
		{
			pCommandList->EndRenderPass();
		}
	}

	void GUIRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void GUIRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void GUIRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			VALIDATE(count == BACK_BUFFER_COUNT);

			for (uint32 i = 0; i < count; i++)
			{
				m_pBackBuffers[i] = MakeSharedRef(ppTextureViews[i]);
			}
		}
	}

	void GUIRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void GUIRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void GUIRenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage)
	{
		m_ModFrameIndex		= modFrameIndex;
		m_BackBufferIndex	= backBufferIndex;

		TArray<Buffer*>& frameBuffersToRemove = m_pBuffersToRemove[m_ModFrameIndex];

		if (!frameBuffersToRemove.IsEmpty())
		{
			for (Buffer* pBuffer : frameBuffersToRemove)
			{
				SAFERELEASE(pBuffer);
			}

			frameBuffersToRemove.Clear();
		}

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

	bool GUIRenderer::CreateBuffers()
	{
		BufferDesc bufferDesc = {};
		bufferDesc.DebugName	= "GUI Params Buffer";
		bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
		bufferDesc.SizeInBytes	= sizeof(GUIParamsData);

		m_pParamsBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);

		return m_pParamsBuffer != nullptr;
	}

	bool GUIRenderer::CreateCommandLists()
	{
		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
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

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DONT_CARE;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "GUI Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_pMainRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return true;
	}
}