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

			//ConstantRangeDesc constantRangeDesc = {};
			//constantRangeDesc.OffsetInBytes = 0;
			//constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;
			//constantRangeDesc.SizeInBytes = sizeof(SPushConstantData);

			PipelineLayoutDesc pPipelineLayoutDesc = { };
			pPipelineLayoutDesc.DebugName = "Tessellator Pipeline Layout";
			pPipelineLayoutDesc.DescriptorSetLayouts = { inDescriptorSetLayoutDesc, outDescriptorSetLayoutDesc };
			//pPipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

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
			pipelineStateDesc.PipelineLayout = MakeSharedRef(m_pPipelineLayout);

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
		SAFERELEASE(m_pInDescriptorSet);
		SAFERELEASE(m_pOutDescriptorSet);
		SAFERELEASE(m_pPipelineLayout);
		SAFERELEASE(m_pDescriptorHeap);
		SAFERELEASE(m_pCommandList);
		SAFERELEASE(m_pCommandAllocator);
		SAFERELEASE(m_pFence);

		ReleaseTessellationBuffers();

		// Release textures
		SAFERELEASE(m_pDummyTexture);
		SAFERELEASE(m_pDummyTextureView);
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


		const uint64 MAX_VERTEX_COUNT = 1500;
		if (pMesh->Vertices.GetSize() < MAX_VERTEX_COUNT)
		{
			// Calculate max tessellation primitiveCount
			const float MaxInnerTessLevel = 24.0f;
			const float MaxOuterTessLevel = 24.0f;
			const float Temporary = 80.0f;
			// Calculate inner faces
			uint64 innerTriCount = 0.0f;
			if (MaxInnerTessLevel > 1.0 && uint32(MaxInnerTessLevel) % 2U == 0U) // Even
			{
				innerTriCount = uint64(6.0f*powf((MaxInnerTessLevel * 0.5f) - 1.0f, 2.0f));
			}
			else // Odd
			{
				innerTriCount = uint64(1.5f * (MaxInnerTessLevel - 1.0f) * (MaxInnerTessLevel - 3.0f) + 1);
			}

			// Calculate outer faces
			uint64 sum = uint64(MaxOuterTessLevel * 3U);
			uint64 outerTriCount = (sum == 3) ? 1 : sum;
			if (MaxInnerTessLevel > 1.0) // Even
			{
				outerTriCount = 3U * MaxInnerTessLevel + sum-6U;
			}

		
			uint64 totalTriCount = innerTriCount + outerTriCount;
			uint64 totalVertexCount = (totalTriCount * 3U) * (pMesh->Indices.GetSize() / 3U) * Temporary;
		
			uint64 newVerticesMaxSize = totalVertexCount * sizeof(Vertex);

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
				meshCalculationData.MaxInnerLevelTess = MaxInnerTessLevel;
				meshCalculationData.MaxOuterLevelTess = MaxOuterTessLevel;
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
					Buffer* buffers[1] = { m_pOutVertexBuffer };
					uint64 offsets[1] = { 0 };
					uint64 sizes[1] = { m_pOutVertexBuffer->GetDesc().SizeInBytes };
					m_pOutDescriptorSet->WriteBufferDescriptors(buffers, offsets, sizes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
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

				m_pCommandList->CopyBuffer(m_pOutVertexBuffer, 0, m_pOutVertexSecondStagingBuffer, 0, newVerticesMaxSize);
				m_pCommandList->CopyBuffer(m_pCalculationDataBuffer, 0, m_pCalculationDataStagingBuffer, 0, sizeof(SCalculationData));

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
			// TODO: Compact this and merge by distance! If merging, change the index too!
			void* pMapped = m_pOutVertexSecondStagingBuffer->Map();
			pMesh->Vertices.Resize(newVerticesMaxSize/sizeof(Vertex));
			memcpy(pMesh->Vertices.GetData(), pMapped, newVerticesMaxSize);
			m_pOutVertexSecondStagingBuffer->Unmap();
			LOG_WARNING("Done");

			SCalculationData calculationData = {};
			pMapped = m_pCalculationDataStagingBuffer->Map();
			memcpy(&calculationData, pMapped, sizeof(SCalculationData));
			m_pCalculationDataStagingBuffer->Unmap();
			LOG_WARNING("Done");

			LOG_WARNING("Compact data...");
			const uint32 triangleCount = calculationData.PrimitiveCounter;
			const uint32 vertexCount = triangleCount * 3U;
			const uint32 indexCount = triangleCount * 3U;

			pMesh->Vertices.Resize(vertexCount);
			TArray<Vertex> vertices = pMesh->Vertices;
			THashTable<Vertex, MeshIndexType> vertexToIndex;

			// Merge duplicated vertices
			pMesh->Indices.Clear();
			pMesh->Indices.Reserve(vertexCount);

			pMesh->Vertices.Clear();
			pMesh->Vertices.Reserve(vertexCount);
			uint32 currentIndex = 0;
			uint32 uniqueVertices = 0;
			for (uint32 i = 0; i < vertices.GetSize(); i++)
			{
				Vertex& vertex = vertices[i];
				auto it = vertexToIndex.find(vertex);
				if (it == vertexToIndex.end())
				{
					vertexToIndex[vertex] = currentIndex;
					pMesh->Vertices.PushBack(vertex);
					pMesh->Indices.PushBack(currentIndex);
					currentIndex++;
					uniqueVertices++;
				}
				else
				{
					uint32 existingIndex = it->second;
					pMesh->Indices.PushBack(existingIndex);
				}
			}
			pMesh->Vertices.Resize(uniqueVertices);

			// Remove the unused elements.
			LOG_WARNING("Mesh PostVertexCount: %d", pMesh->Vertices.GetSize());
			LOG_WARNING("Done");
		
			LOG_WARNING("Tessellation Complete");
		}
		else {
			LOG_WARNING("Tessellation Ignored: VertexCount exceeded %d", MAX_VERTEX_COUNT);
		}

	}

	void MeshTessellator::ReleaseTessellationBuffers()
	{
		// Release buffers
		SAFERELEASE(m_pInIndicesBuffer);
		SAFERELEASE(m_pInIndicesStagingBuffer);
		SAFERELEASE(m_pInVertexBuffer);
		SAFERELEASE(m_pInVertexStagingBuffer);

		SAFERELEASE(m_pOutVertexBuffer);
		SAFERELEASE(m_pOutVertexFirstStagingBuffer);
		SAFERELEASE(m_pOutVertexSecondStagingBuffer);

		SAFERELEASE(m_pCalculationDataBuffer);
		SAFERELEASE(m_pCalculationDataStagingBuffer);
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
		uint64 stagingBufferSize = 0;
		uint64 bufferSize = 0;
		if ((*inStagingBuffer) == nullptr || (*inStagingBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*inStagingBuffer) != nullptr)
				SAFERELEASE((*inStagingBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			stagingBufferSize = size;

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

			bufferSize = size;

			(*inBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}

		if (stagingBufferSize || bufferSize)
		{
			LOG_ERROR("Allocated %s: %d bytes & %d bytes", name.c_str(), stagingBufferSize, bufferSize);
		}

		pCommandList->CopyBuffer((*inStagingBuffer), 0, (*inBuffer), 0, size);
	}

	void MeshTessellator::CreateAndClearOutBuffer(CommandList* pCommandList, Buffer** outBuffer, Buffer** outFirstStagingBuffer, Buffer** outSecondStagingBuffer, uint64 size, uint32 clearData, const String& name)
	{
		uint64 stagingBufferSize = 0;
		uint64 bufferSize0 = 0;
		uint64 bufferSize1 = 0;

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

		if (stagingBufferSize || bufferSize0 || bufferSize1)
		{
			LOG_ERROR("Allocated %s: %d bytes & %d bytes &d", name.c_str(), stagingBufferSize, bufferSize0, bufferSize1);
		}
	}
}
