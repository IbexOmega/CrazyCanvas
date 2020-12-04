#include "Resources/MeshTessellator.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/PipelineStateManager.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	MeshTessellator MeshTessellator::s_Instance;

	void MeshTessellator::Init()
	{
		// Create dummy render target because graphics pipeline must RENDER to something
		CreateDummyRenderTarget();

		// Create Command List
		{
			m_pCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Tessellator Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			CommandListDesc commandListDesc = { };
			commandListDesc.DebugName = "Tessellator Graphics Command List";
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			m_pCommandList = RenderAPI::GetDevice()->CreateCommandList(m_pCommandAllocator, &commandListDesc);
		}

		//Create Fence
		{
			FenceDesc fenceDesc = {};
			fenceDesc.DebugName = "Tessellator Fence";
			fenceDesc.InitalValue = 0;
			m_pFence = RenderAPI::GetDevice()->CreateFence(&fenceDesc);
		}

		// Create Descriptor Heap
		{
			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 2;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName = "Tessellator Descriptor Heap";
			descriptorHeapDesc.DescriptorSetCount = 2;
			descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

			m_pDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		}

		// Create Pipeline Layout
		{
			// Original vertices
			DescriptorBindingDesc inVerticesBinding = { };
			inVerticesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			inVerticesBinding.DescriptorCount = 1;
			inVerticesBinding.Binding = 0;
			inVerticesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;
			
			DescriptorBindingDesc counterBinding = { };
			counterBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			counterBinding.DescriptorCount = 1;
			counterBinding.Binding = 1;
			counterBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			DescriptorSetLayoutDesc inDescriptorSetLayoutDesc = { };
			inDescriptorSetLayoutDesc.DescriptorBindings =
			{
				inVerticesBinding,
				counterBinding
			};

			DescriptorBindingDesc outVerticesBinding = { };
			outVerticesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			outVerticesBinding.DescriptorCount = 1;
			outVerticesBinding.Binding = 0;
			outVerticesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			DescriptorBindingDesc outIndicesBinding = { };
			outIndicesBinding.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			outIndicesBinding.DescriptorCount = 1;
			outIndicesBinding.Binding = 1;
			outIndicesBinding.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			DescriptorSetLayoutDesc outDescriptorSetLayoutDesc = { };
			outDescriptorSetLayoutDesc.DescriptorBindings =
			{
				outVerticesBinding,
				outIndicesBinding
			};

			ConstantRangeDesc constantRangeDesc = {};
			constantRangeDesc.OffsetInBytes = 0;
			constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;
			constantRangeDesc.SizeInBytes = sizeof(SPushConstantData);

			PipelineLayoutDesc pPipelineLayoutDesc = { };
			pPipelineLayoutDesc.DebugName = "Tessellator Pipeline Layout";
			pPipelineLayoutDesc.DescriptorSetLayouts = { inDescriptorSetLayoutDesc, outDescriptorSetLayoutDesc };
			pPipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

			m_pPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pPipelineLayoutDesc);

			m_pInDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Tessellator In Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);
			m_pOutDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Tessellator Out Descriptor Set", m_pPipelineLayout, 1, m_pDescriptorHeap);
		}

		// Create Pipeline State
		{
			RenderPassSubpassDesc subpassDesc = {};

			RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
			subpassDependencyDesc.SrcSubpass = EXTERNAL_SUBPASS;
			subpassDependencyDesc.DstSubpass = 0;
			subpassDependencyDesc.SrcAccessMask = 0;
			subpassDependencyDesc.DstAccessMask = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			subpassDependencyDesc.SrcStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
			subpassDependencyDesc.DstStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

			RenderPassAttachmentDesc colorAttachmentDesc = {};
			colorAttachmentDesc.Format = EFormat::FORMAT_R8_UINT;
			colorAttachmentDesc.SampleCount = 1;
			colorAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_DONT_CARE;
			colorAttachmentDesc.StoreOp = EStoreOp::STORE_OP_DONT_CARE;
			colorAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
			colorAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
			colorAttachmentDesc.InitialState = ETextureState::TEXTURE_STATE_UNKNOWN;
			colorAttachmentDesc.FinalState = ETextureState::TEXTURE_STATE_GENERAL;

			RenderPassDesc renderPassDesc = {};
			renderPassDesc.DebugName = "Tessellator Render Pass";
			renderPassDesc.Attachments = { colorAttachmentDesc };
			renderPassDesc.Subpasses = { subpassDesc };
			renderPassDesc.SubpassDependencies = { subpassDependencyDesc };
			m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

			ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
			pipelineStateDesc.DebugName = "Player Renderer Pipeline Back Cull State";
			pipelineStateDesc.RenderPass = m_RenderPass;
			pipelineStateDesc.PipelineLayout = m_pPipelineLayout;

			pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_PATCH_LIST;

			pipelineStateDesc.RasterizerState.LineWidth = 1.f;
			pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
			pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;

			pipelineStateDesc.DepthStencilState = {};
			pipelineStateDesc.DepthStencilState.DepthTestEnable = false;
			pipelineStateDesc.DepthStencilState.DepthWriteEnable = false;

			GUID_Lambda vertexShader = ResourceManager::LoadShaderFromFile("Tessellation/Passthrough.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");
			GUID_Lambda pixelShader = ResourceManager::LoadShaderFromFile("Tessellation/Passthrough.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");
			GUID_Lambda controlShader = ResourceManager::LoadShaderFromFile("Tessellation/Passthrough.tesc", FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");
			GUID_Lambda evaluationShader = ResourceManager::LoadShaderFromFile("Tessellation/Passthrough.tese", FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");
			GUID_Lambda geomShader = ResourceManager::LoadShaderFromFile("Tessellation/Output.geom", FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");

			pipelineStateDesc.VertexShader.ShaderGUID = vertexShader;
			pipelineStateDesc.HullShader.ShaderGUID = controlShader;
			pipelineStateDesc.DomainShader.ShaderGUID = evaluationShader;
			pipelineStateDesc.GeometryShader.ShaderGUID = geomShader;
			pipelineStateDesc.PixelShader.ShaderGUID = pixelShader;

			m_pPipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		}
	}

	void MeshTessellator::Release()
	{
		m_pInDescriptorSet->Release();
		m_pOutDescriptorSet->Release();
		m_pDescriptorHeap->Release();
		m_pPipelineLayout->Release();
		m_pCommandList->Release();
		m_pCommandAllocator->Release();
		m_pFence->Release();

		// Release buffers
		m_pInIndicesBuffer->Release();
		m_pInIndicesStagingBuffer->Release();
		m_pInVertexBuffer->Release();
		m_pInVertexStagingBuffer->Release();

		m_pOutIndicesBuffer->Release();
		m_pOutIndicesFirstStagingBuffer->Release();
		m_pOutIndicesSecondStagingBuffer->Release();

		m_pOutVertexBuffer->Release();
		m_pOutVertexFirstStagingBuffer->Release();
		m_pOutVertexSecondStagingBuffer->Release();

		m_pCalculationDataBuffer->Release();
		m_pCalculationDataStagingBuffer->Release();

		// Release textures
		m_pDummyTexture->Release();
		m_pDummyTextureView->Release();
	}

	void MeshTessellator::Tessellate(Mesh* pMesh)
	{
		LOG_WARNING("Tesselate...");

		/* TODO:
		*	1. [x] Make input buffers for the original vertices and indices. 
		*	1. [ ] Make result buffers for the new vertices and indices, have them be cleard and set to MAX_UINT32 before calling dispatch.
		*	2. [x] Write to the descriptor sets!
		*	3. [x] Call dispatch
		*	4. [ ] Compact buffers (Remove all elements which did not have any change and merge vertices by distance)
		*	5. [ ] Copy data from the new compacted buffers to pMesh
		**/

		SPushConstantData pushConstantData = {};
		pushConstantData.triangleCount = pMesh->Indices.GetSize() / 3;
		pushConstantData.maxTesselationLevels = 2;
		pushConstantData.outerBorder = 3; // Not counting the corners! One outer edge on the triangle will be, o-x-x-x-o, if the outerBorder is 3.

		// TODO: Change this to be the correct size, if assuming all triangle will be fully tessellated!
		const uint32 MaxInnerTessellationLevel = 6;
		const uint32 MaxOuterTessellationLevel = 6;

		uint64 newVerticesMaxSize = (pushConstantData.triangleCount * 12 * MaxInnerTessellationLevel * MaxOuterTessellationLevel) * sizeof(Vertex);
		uint64 newIndicesMaxSize = (pushConstantData.triangleCount * 12 * MaxInnerTessellationLevel * MaxOuterTessellationLevel) * sizeof(MeshIndexType);

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass;
		beginRenderPassDesc.ppRenderTargets = &m_pDummyTextureView;
		beginRenderPassDesc.pDepthStencil = nullptr;
		beginRenderPassDesc.RenderTargetCount = 1;
		beginRenderPassDesc.Width = 1;
		beginRenderPassDesc.Height = 1;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = nullptr;
		beginRenderPassDesc.ClearColorCount = 0;
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;

		Viewport viewport = {};
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.Width = 1;
		viewport.Height = 1;
		viewport.x = 0.0f;
		viewport.y = 0;

		ScissorRect scissorRect = {};
		scissorRect.Width = 1;
		scissorRect.Height = 1;

		LOG_WARNING("Mesh PreVertexCount: %d", pMesh->Vertices.GetSize());
		LOG_WARNING("Create buffers...");
		static uint64 signalValue = 0;
		{
			m_pCommandAllocator->Reset();
			m_pCommandList->Begin(nullptr);

			// ------- Create buffers if needed and copy data to them ------

			// Create buffer for tessellation calculations
			SCalculationData meshCalculationData = {};
			meshCalculationData.ScaleMatrix = glm::scale(pMesh->DefaultScale);
			meshCalculationData.PrimitiveCounter = 0;
			uint64 size = sizeof(SCalculationData);
			CreateAndCopyInBuffer(m_pCommandList, &m_pCalculationDataBuffer, &m_pCalculationDataStagingBuffer, (void*)&meshCalculationData, size, "Tessellator Calculation Data Buffer", FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER);

			// In Buffers
			void* data = pMesh->Vertices.GetData();
			size = pMesh->Vertices.GetSize() * sizeof(Vertex);
			CreateAndCopyInBuffer(m_pCommandList, &m_pInVertexBuffer, &m_pInVertexStagingBuffer, pMesh->Vertices.GetData(), size, "Tessellator In Vertex Buffer", FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER);

			data = pMesh->Indices.GetData();
			size = pMesh->Indices.GetSize() * sizeof(MeshIndexType);
			CreateAndCopyInBuffer(m_pCommandList, &m_pInIndicesBuffer, &m_pInIndicesStagingBuffer, pMesh->Indices.GetData(), size, "Tessellator In Index Buffer", FBufferFlag::BUFFER_FLAG_INDEX_BUFFER);

			// Out Buffers
			CreateAndClearOutBuffer(m_pCommandList, &m_pOutVertexBuffer, &m_pOutVertexFirstStagingBuffer, &m_pOutVertexSecondStagingBuffer, newVerticesMaxSize, UINT32_MAX, "Tessellator Out Vertex Buffer");
			CreateAndClearOutBuffer(m_pCommandList, &m_pOutIndicesBuffer, &m_pOutIndicesFirstStagingBuffer, &m_pOutIndicesSecondStagingBuffer, newIndicesMaxSize, UINT32_MAX, "Tessellator Out Index Buffer");

			m_pCommandList->End();

			signalValue++;
			RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(
				&m_pCommandList, 1,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN,
				nullptr, 0,
				m_pFence, signalValue);

			m_pFence->Wait(signalValue, UINT64_MAX);
		}

		LOG_WARNING("Done");

		LOG_WARNING("Write descriptors...");
		// Write to descriptor sets
		{
			// In descriptor set
			{
				Buffer* buffers[2] = { m_pInVertexBuffer, m_pCalculationDataBuffer };
				uint64 offsets[2] = { 0, 0 };
				uint64 sizes[2] = { m_pInVertexBuffer->GetDesc().SizeInBytes, m_pCalculationDataBuffer->GetDesc().SizeInBytes };
				m_pInDescriptorSet->WriteBufferDescriptors(buffers, offsets, sizes, 0, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			}
			
			// Out descriptor set
			{
				Buffer* buffers[2] = { m_pOutVertexBuffer, m_pOutIndicesBuffer };
				uint64 offsets[2] = { 0, 0 };
				uint64 sizes[2] = { m_pOutVertexBuffer->GetDesc().SizeInBytes, m_pOutIndicesBuffer->GetDesc().SizeInBytes };
				m_pOutDescriptorSet->WriteBufferDescriptors(buffers, offsets, sizes, 0, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			}
		}
		LOG_WARNING("Done");

		LOG_WARNING("Tessellate...");
		{
			m_pCommandAllocator->Reset();
			m_pCommandList->Begin(nullptr);
			m_pCommandList->BeginRenderPass(&beginRenderPassDesc);
			m_pCommandList->SetViewports(&viewport, 0, 1);
			m_pCommandList->SetScissorRects(&scissorRect, 0, 1);

			// ------- Graphics pipeline -------
			m_pCommandList->BindDescriptorSetGraphics(m_pInDescriptorSet, m_pPipelineLayout, 0);
			m_pCommandList->BindDescriptorSetGraphics(m_pOutDescriptorSet, m_pPipelineLayout, 1);

			m_pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_pPipelineStateID));
			m_pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlag::SHADER_STAGE_FLAG_ALL, (void*)&pushConstantData, sizeof(pushConstantData), 0);
			
			m_pCommandList->BindIndexBuffer(m_pInIndicesBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

			// Render
			m_pCommandList->DrawIndexInstanced(pMesh->Indices.GetSize(), 1, 0, 0, 0);

			m_pCommandList->EndRenderPass();

			static constexpr const PipelineMemoryBarrierDesc MEMORY_BARRIER
			{
				.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
				.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
			};

			m_pCommandList->PipelineMemoryBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY,
				&MEMORY_BARRIER,
				1);

			m_pCommandList->CopyBuffer(m_pOutIndicesBuffer, 0, m_pOutIndicesSecondStagingBuffer, 0, newIndicesMaxSize);
			m_pCommandList->CopyBuffer(m_pOutVertexBuffer, 0, m_pOutVertexSecondStagingBuffer, 0, newVerticesMaxSize);

			m_pCommandList->End();

			signalValue++;
			RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(
				&m_pCommandList, 1,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
				m_pFence, signalValue-1,
				m_pFence, signalValue);

			m_pFence->Wait(signalValue, UINT64_MAX);
		}
		LOG_WARNING("Done");

		// Copy data to CPU

		LOG_WARNING("Copy over new data...");
		// TODO: Compact this! Check for elements which are UINT32_MAX, they should be removed!
		void* pMapped = m_pOutIndicesSecondStagingBuffer->Map();
		pMesh->Indices.Resize(newIndicesMaxSize/sizeof(MeshIndexType));
		memcpy(pMesh->Indices.GetData(), pMapped, newIndicesMaxSize);
		m_pOutIndicesSecondStagingBuffer->Unmap();

		// TODO: Compact this and merge by distance! If merging, change the index too!
		pMapped = m_pOutVertexSecondStagingBuffer->Map();
		pMesh->Vertices.Resize(newVerticesMaxSize/sizeof(Vertex));
		memcpy(pMesh->Vertices.GetData(), pMapped, newVerticesMaxSize);
		m_pOutVertexSecondStagingBuffer->Unmap();
		LOG_WARNING("Done");

		LOG_WARNING("Compact data...");
		uint32 emptyCount = 0;

		THashTable<glm::vec3, uint32> vertexToIndexMap;
		// Move all elements to the left.
		for (uint32 i = 0; i < pMesh->Indices.GetSize(); i++)
		{
			uint32 index = pMesh->Indices[i];
			if (index == UINT32_MAX)
				emptyCount++;
			else
			{
				uint32 newI = i - emptyCount;
				pMesh->Indices[newI] = newI;
				pMesh->Vertices[newI] = pMesh->Vertices[index];
			}
		}

		// Remove the unused elements.
		pMesh->Indices.Resize(pMesh->Indices.GetSize() - emptyCount);
		pMesh->Vertices.Resize(pMesh->Vertices.GetSize() - emptyCount);
		LOG_WARNING("Mesh PostVertexCount: %d", pMesh->Vertices.GetSize());
		LOG_WARNING("Done");
		
		LOG_WARNING("Tessellation Complete");
	}

	void MeshTessellator::CreateDummyRenderTarget()
	{
		TextureDesc textureDesc = {};
		textureDesc.DebugName = "Dummy Texture Tessellation";
		textureDesc.Type = ETextureType::TEXTURE_TYPE_2D;
		textureDesc.ArrayCount = 1;
		textureDesc.Depth = 1;
		textureDesc.Flags = FTextureFlag::TEXTURE_FLAG_RENDER_TARGET;
		textureDesc.Width = 1;
		textureDesc.Height = 1;
		textureDesc.Format = EFormat::FORMAT_R8_UINT;
		textureDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
		textureDesc.Miplevels = 1;
		textureDesc.SampleCount = 1;
		m_pDummyTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);

		if (m_pDummyTexture == nullptr)
		{
			LOG_ERROR("[MeshTessellator]: Failed to create texture for \"Tessellator\"");
		}

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.pTexture = m_pDummyTexture;
		textureViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET;
		textureViewDesc.Format = textureDesc.Format;
		textureViewDesc.Type = ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.Miplevel = 0;
		textureViewDesc.MiplevelCount = textureDesc.Miplevels;
		textureViewDesc.ArrayCount = 1;
		m_pDummyTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		if (m_pDummyTextureView == nullptr)
		{
			LOG_ERROR("[MeshTessellator]: Failed to create texture view for \"Tessellator\"");
		}
	}

	void MeshTessellator::CreateAndCopyInBuffer(CommandList* pCommandList, Buffer** inBuffer, Buffer** inStagingBuffer, void* data, uint64 size, const String& name, FBufferFlags flags)
	{
		if ((*inStagingBuffer) == nullptr || (*inStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*inStagingBuffer) != nullptr)
				SAFERELEASE((*inStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			(*inStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = (*inStagingBuffer)->Map();
		memcpy(pMapped, data, size);
		(*inStagingBuffer)->Unmap();

		if ((*inBuffer) == nullptr || (*inBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*inBuffer) != nullptr)
				SAFERELEASE((*inBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name;
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | flags;
			bufferDesc.SizeInBytes = size;

			(*inBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer((*inStagingBuffer), 0, (*inBuffer), 0, size);
	}

	void MeshTessellator::CreateAndClearOutBuffer(CommandList* pCommandList, Buffer** outBuffer, Buffer** outFirstStagingBuffer, Buffer** outSecondStagingBuffer, uint64 size, uint32 clearData, const String& name)
	{
		if ((*outFirstStagingBuffer) == nullptr || (*outFirstStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outFirstStagingBuffer) != nullptr)
				SAFERELEASE((*outFirstStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " First Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			(*outFirstStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		void* pMapped = (*outFirstStagingBuffer)->Map();
		memset(pMapped, clearData, size);
		(*outFirstStagingBuffer)->Unmap();

		if ((*outBuffer) == nullptr || (*outBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outBuffer) != nullptr)
				SAFERELEASE((*outBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name;
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC | FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes = size;

			(*outBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		pCommandList->CopyBuffer((*outFirstStagingBuffer), 0, (*outBuffer), 0, size);

		if ((*outSecondStagingBuffer) == nullptr || (*outSecondStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*outSecondStagingBuffer) != nullptr)
				SAFERELEASE((*outSecondStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Second Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = size;

			(*outSecondStagingBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}
	}
}
