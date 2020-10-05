#include "PreCompiled.h"
#include "Rendering/PaintMaskRenderer.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraph.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/EntityMaskManager.h"

#include "Game/ECS/Components/Misc/MeshPaintComponent.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Resources/ResourceManager.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{
	PaintMaskRenderer::PaintMaskRenderer()
	{
	}

	PaintMaskRenderer::~PaintMaskRenderer()
	{
		if (m_ppRenderCommandLists && m_ppRenderCommandAllocators)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppRenderCommandLists[b]);
				SAFERELEASE(m_ppRenderCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppRenderCommandLists);
			SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
		}
	}

	bool PaintMaskRenderer::init(GraphicsDevice* pGraphicsDevice, uint32 backBufferCount)
	{
		m_BackBuffers.Resize(backBufferCount);
		m_BackBufferCount = backBufferCount;

		m_pGraphicsDevice = pGraphicsDevice;

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateBuffers())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create buffers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSet())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create Shaders");
			return false;
		}
		
		return false;
	}

	bool PaintMaskRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateCommandLists())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create render command lists");
			return false;
		}

		if (!CreateRenderPass(pPreInitDesc))
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create PipelineState");
			return false;
		}
		
		return true;
	}

	void PaintMaskRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			for (uint32 i = 0; i < count; i++)
			{
				m_BackBuffers[i] = MakeSharedRef(ppTextureViews[i]);
			}
		}

		if (resourceName == "BRUSH_MASK")
		{
			// This should not be necessary, because we already know that the brush mask texture is not back buffer bound.
			Sampler* sampler = Sampler::GetLinearSampler();
			if (!m_BrushMaskDescriptorSet.IsEmpty())
			{
				if (backBufferBound)
				{
					// If it is back buffer bound then create copies for each backbuffer
					uint32 backBufferCount = m_BackBuffers.GetSize();
					m_BrushMaskDescriptorSet.Resize(backBufferCount);
					for (uint32 b = 0; b < backBufferCount; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						m_BrushMaskDescriptorSet[b] = descriptorSet;

						descriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
					}
				}
				else
				{
					// Else create as many as requested
					m_BrushMaskDescriptorSet.Resize(count);
					for (uint32 b = 0; b < count; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						m_BrushMaskDescriptorSet[b] = descriptorSet;

						descriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
					}
				}
			}
			else
			{
				// Else the resource exists
				if (backBufferBound)
				{
					uint32 backBufferCount = m_BackBuffers.GetSize();
					if (m_BrushMaskDescriptorSet.GetSize() == backBufferCount)
					{
						for (uint32 b = 0; b < backBufferCount; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = m_BrushMaskDescriptorSet[b];
							descriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
						}
					}
					else
					{
						LOG_ERROR("[Paint Mask Renderer]: Backbuffer count does not match the amount of descriptors to update for resource \"%s\"", resourceName.c_str());
					}
				}
				else
				{
					if (m_BrushMaskDescriptorSet.GetSize() == count)
					{
						for (uint32 b = 0; b < count; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = m_BrushMaskDescriptorSet[b];
							descriptorSet->WriteTextureDescriptors(&ppTextureViews[b], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
						}
					}
					else
					{
						LOG_ERROR("[Paint Mask Renderer]: Buffer count changed between calls to UpdateBufferResource for resource \"%s\"", resourceName.c_str());
					}
				}
			}
		}
	}

	void PaintMaskRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		if (count == 1 || backBufferBound)
		{
			auto bufferIt = m_BufferResourceNameDescriptorSetsMap.find(resourceName);
			if (bufferIt == m_BufferResourceNameDescriptorSetsMap.end())
			{
				// If resource doesn't exist, create descriptor and write it
				TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_BufferResourceNameDescriptorSetsMap[resourceName];
				if (backBufferBound)
				{
					// If it is backbufferbound then create copies for each backbuffer
					uint32 backBufferCount = m_BackBuffers.GetSize();
					descriptorSets.Resize(backBufferCount);
					for (uint32 b = 0; b < backBufferCount; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						descriptorSets[b] = descriptorSet;

						descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
					}
				}
				else
				{
					// Else create as many as requested
					descriptorSets.Resize(count);
					for (uint32 b = 0; b < count; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						descriptorSets[b] = descriptorSet;

						descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
					}
				}
			}
			else
			{
				// Else the resource exists
				TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_BufferResourceNameDescriptorSetsMap[resourceName];
				if (backBufferBound)
				{
					uint32 backBufferCount = m_BackBuffers.GetSize();
					if (descriptorSets.GetSize() == backBufferCount)
					{
						for (uint32 b = 0; b < backBufferCount; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
							descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
						}
					}
					else
					{
						LOG_ERROR("[Paint Mask Renderer]: Backbuffer count does not match the amount of descriptors to update for resource \"%s\"", resourceName.c_str());
					}

				}
				else
				{
					if (descriptorSets.GetSize() == count)
					{
						for (uint32 b = 0; b < count; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
							descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
						}
					}
					else
					{
						LOG_ERROR("[Paint Mask Renderer]: Buffer count changed between calls to UpdateBufferResource for resource \"%s\"", resourceName.c_str());
					}

				}
			}
		}
	}

	void PaintMaskRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}
	
	void PaintMaskRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		//Kallas när ett DrawArg har uppdaterats i RenderGraphen, du kan anta att DrawArgMasken överenstämmer med det som är angivet i RenderGraphen
		for (uint32 i = 0; i < pDrawArgs->InstanceCount; i++)
		{
			DrawArgExtensionGroup* extensionGroup = pDrawArgs->ppExtensionGroups[i];

			// TODO: We can assume there is only one extension, because this render stage has a DrawArgMask of 2 which is one specific extension.
			uint32 numExtensions = extensionGroup->ExtensionCount;
			for (uint32 e = 0; e < numExtensions; e++)
			{
				uint32 mask = extensionGroup->pExtensionMasks[e];
				DrawArgExtensionData& extension = extensionGroup->pExtensions[e];

				for (uint32 t = 0; t < extension.TextureCount; t++)
				{
					Texture* texture = extension.ppTextures[t];
					TextureView* textureView = extension.ppTextureViews[t];
					// TODO: Make the render function use each texture as a render target.
				}
			}
		}
	}

	void PaintMaskRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		TSharedRef<const TextureView> backBuffer = m_BackBuffers[backBufferIndex];
		uint32 width = backBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height = backBuffer->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets = &backBuffer;
		beginRenderPassDesc.RenderTargetCount = 1;
		//beginRenderPassDesc.pDepthStencil = m_DepthStencilBuffer.Get();
		beginRenderPassDesc.Width = width;
		beginRenderPassDesc.Height = height;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = nullptr;
		beginRenderPassDesc.ClearColorCount = 0;
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];

		
		//TODO: Do this once if no data is avaliable
		//if (m_LineGroups.size() == 0 && m_Verticies.GetSize() == 0)
		//{
		//	m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		//	pCommandList->Begin(nullptr);
		//	//Begin and End RenderPass to transition Texture State (Lazy)
		//	pCommandList->BeginRenderPass(&beginRenderPassDesc);
		//	pCommandList->EndRenderPass();
		//
		//	pCommandList->End();
		//
		//	(*ppFirstExecutionStage) = pCommandList;
		//	return;
		//}

		m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.Width = (float32)width;
		viewport.Height = -(float32)height;
		viewport.x = 0.0f;
		viewport.y = (float32)height;
		pCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width = width;
		scissorRect.Height = height;
		pCommandList->SetScissorRects(&scissorRect, 0, 1);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));

		if (m_BufferResourceNameDescriptorSetsMap.contains(PER_FRAME_BUFFER))
		{
			auto& descriptorSets = m_BufferResourceNameDescriptorSetsMap[PER_FRAME_BUFFER];
			pCommandList->BindDescriptorSetGraphics(descriptorSets[0].Get(), m_PipelineLayout.Get(), 0);
		}

		if (!m_BrushMaskDescriptorSet.IsEmpty())
		{
			auto& descriptorSetBrush = m_BrushMaskDescriptorSet[backBufferIndex];
			pCommandList->BindDescriptorSetGraphics(descriptorSetBrush.Get(), m_PipelineLayout.Get(), 0);
		}

		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet.Get(), m_PipelineLayout.Get(), 1);

		//pCommandList->DrawInstanced(drawCount, drawCount, 0, 0);

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}

	bool PaintMaskRenderer::CreateCopyCommandList()
	{
		m_CopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Paint Mask Renderer Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
		if (!m_CopyCommandAllocator)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName = "Paint Mask Renderer Copy Command List";
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_CopyCommandList = m_pGraphicsDevice->CreateCommandList(m_CopyCommandAllocator.Get(), &commandListDesc);

		return m_CopyCommandList != nullptr;
	}

	bool PaintMaskRenderer::CreateBuffers()
	{
		BufferDesc uniformCopyBufferDesc = {};
		uniformCopyBufferDesc.DebugName = "Paint Mask Renderer Transform Copy Buffer";
		uniformCopyBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		uniformCopyBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
		uniformCopyBufferDesc.SizeInBytes = sizeof(glm::mat4);

		uint32 backBufferCount = m_BackBuffers.GetSize();
		m_TransformCopyBuffers.Resize(backBufferCount);
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			TSharedRef<Buffer> uniformBuffer = m_pGraphicsDevice->CreateBuffer(&uniformCopyBufferDesc);
			if (uniformBuffer != nullptr)
			{
				m_TransformCopyBuffers[b] = uniformBuffer;
			}
			else
			{
				return false;
			}
		}

		BufferDesc uniformBufferDesc = {};
		uniformBufferDesc.DebugName = "Paint Mask Renderer Transform Buffer";
		uniformBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
		uniformBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
		uniformBufferDesc.SizeInBytes = uniformCopyBufferDesc.SizeInBytes;

		m_TransformBuffer = m_pGraphicsDevice->CreateBuffer(&uniformBufferDesc);
		return m_TransformBuffer != nullptr;
	}

	bool PaintMaskRenderer::CreatePipelineLayout()
	{
		// PerFrameBuffer (Binding 0) and Transform (Binding 1)
		DescriptorBindingDesc perFrameBufferDesc = {};
		perFrameBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount = 2;
		perFrameBufferDesc.Binding = 0;
		perFrameBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		// Brush mask texture
		DescriptorBindingDesc brushMaskDesc = {};
		brushMaskDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		brushMaskDesc.DescriptorCount = 1;
		brushMaskDesc.Binding = 0;
		brushMaskDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Draw Args (No Extension, only Vertices)
		DescriptorBindingDesc ssboBindingDesc = {};
		ssboBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		ssboBindingDesc.DescriptorCount = 1;
		ssboBindingDesc.Binding = 0;
		ssboBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { perFrameBufferDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { brushMaskDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { ssboBindingDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Paint Mask Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };

		m_PipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool PaintMaskRenderer::CreateDescriptorSet()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 1;
		descriptorCountDesc.ConstantBufferDescriptorCount = 2;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Paint Mask Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 64;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_DescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Descriptor Set", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());

		return m_DescriptorSet != nullptr;
	}

	bool PaintMaskRenderer::CreateShaders()
	{
		m_VertexShaderGUID = ResourceManager::LoadShaderFromFile("/MeshPainting/Unwrap.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderGUID = ResourceManager::LoadShaderFromFile("/MeshPainting/Unwrap.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderGUID != GUID_NONE && m_PixelShaderGUID != GUID_NONE;
	}

	bool PaintMaskRenderer::CreateCommandLists()
	{
		m_ppRenderCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppRenderCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Paint Mask Renderer Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Paint Mask Renderer Render Command List " + std::to_string(b);
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppRenderCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

			if (!m_ppRenderCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool PaintMaskRenderer::CreateRenderPass(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format = EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount = 1;
		colorAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		//colorAttachmentDesc.InitialState = ETextureState::;
		//colorAttachmentDesc.FinalState = pBackBufferAttachmentDesc->FinalState;

		//RenderPassAttachmentDesc depthAttachmentDesc = *pDepthStencilAttachmentDesc;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		//subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass = EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass = 0;
		subpassDependencyDesc.SrcAccessMask = 0;
		subpassDependencyDesc.DstAccessMask = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName = "Paint Mask Renderer Render Pass";
		renderPassDesc.Attachments = { colorAttachmentDesc };
		renderPassDesc.Subpasses = { subpassDesc };
		renderPassDesc.SubpassDependencies = { subpassDependencyDesc };

		m_RenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool PaintMaskRenderer::CreatePipelineState()
	{
		m_PipelineStateID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID);

		THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
		pixelShaderToPipelineStateMap.insert({ m_PixelShaderGUID, m_PipelineStateID });
		m_ShadersIDToPipelineStateIDMap.insert({ m_VertexShaderGUID, pixelShaderToPipelineStateMap });

		return true;
	}

	uint64 PaintMaskRenderer::InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader)
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName = "Paint Mask Renderer Pipeline State";
		pipelineStateDesc.RenderPass = m_RenderPass;
		pipelineStateDesc.PipelineLayout = m_PipelineLayout;

		pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_LINE;
		pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;

		pipelineStateDesc.DepthStencilState = {};
		pipelineStateDesc.DepthStencilState.DepthTestEnable = false;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable = false;

		pipelineStateDesc.BlendState.BlendAttachmentStates =
		{
			{
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A,
				true
			}
		};

		pipelineStateDesc.VertexShader.ShaderGUID = vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID = pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}
}