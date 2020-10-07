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
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

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

				for (TSharedRef<DeviceChild> pDeviceChild : m_pDeviceResourcesToDestroy[b])
				{
					SAFERELEASE(pDeviceChild);
				}
			}

			SAFEDELETE_ARRAY(m_ppRenderCommandLists);
			SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
			m_pDeviceResourcesToDestroy.Clear();
		}
	}

	bool PaintMaskRenderer::init(GraphicsDevice* pGraphicsDevice, uint32 backBufferCount)
	{
		m_BackBuffers.Resize(backBufferCount);
		m_BackBufferCount = backBufferCount;

		m_pDeviceResourcesToDestroy.Resize(m_BackBufferCount);

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
		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			for (uint32 i = 0; i < count; i++)
			{
				m_BackBuffers[i] = MakeSharedRef(ppTextureViews[i]);
			}
		}

		if (resourceName == "BRUSH_MASK")
		{
			VALIDATE(count == 1);
			VALIDATE(backBufferBound == false);

			// This should not be necessary, because we already know that the brush mask texture is not back buffer bound.
			Sampler* sampler = Sampler::GetLinearSampler();
			if (!m_BrushMaskDescriptorSet.has_value())
			{
				m_BrushMaskDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
				m_BrushMaskDescriptorSet.value()->WriteTextureDescriptors(&ppTextureViews[0], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
			}
			else
			{
				if (m_BrushMaskDescriptorSet.has_value())
				{
					m_BrushMaskDescriptorSet.value()->WriteTextureDescriptors(&ppTextureViews[0], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
				}
				else
				{
					LOG_ERROR("[Paint Mask Renderer]: Buffer count changed between calls to UpdateBufferResource for resource \"%s\"", resourceName.c_str());
				}
			}
		}
	}

	void PaintMaskRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		if (count == 1 || backBufferBound)
		{
			if (resourceName == PER_FRAME_BUFFER)
			{
				if (!m_PerFrameBufferDescriptorSets.has_value())
				{
					m_PerFrameBufferDescriptorSets = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom PerFrameBuffer Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
					m_PerFrameBufferDescriptorSets.value()->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
				}
				else
				{
					if (m_BrushMaskDescriptorSet.has_value())
					{
						m_BrushMaskDescriptorSet.value()->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
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
		m_pDrawArgs = pDrawArgs;
		
		uint32 backBufferCount = m_BackBuffers.GetSize();
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			if (m_VerticesDescriptorSets.IsEmpty())
				m_VerticesDescriptorSets.Resize(backBufferCount);

			if (m_TransformDescriptorSets.IsEmpty())
				m_TransformDescriptorSets.Resize(backBufferCount);

			// Remove all descriptor sets for the vertices.
			for (TSharedRef<DescriptorSet> descriptorSet : m_VerticesDescriptorSets[b])
				m_pDeviceResourcesToDestroy[b].PushBack(std::move(descriptorSet));
			m_VerticesDescriptorSets[b].Clear();

			// Remove all previous descriptor sets for the transforms.
			for (auto& drawArgs : m_TransformDescriptorSets[b])
			{
				for (TSharedRef<DescriptorSet> descriptorSet : drawArgs)
					m_pDeviceResourcesToDestroy[b].PushBack(std::move(descriptorSet));
				drawArgs.Clear();
			}
			m_TransformDescriptorSets[b].Resize(count);

			for (uint32 drawArgIndex = 0; drawArgIndex < count; drawArgIndex++)
			{
				const DrawArg& drawArg = pDrawArgs[drawArgIndex];

				// Create new descriptor sets for the vertices.
				{
					TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Vertex Buffer Descriptor Set", m_PipelineLayout.Get(), 2, m_DescriptorHeap.Get());
					uint64 size = drawArg.pVertexBuffer->GetDesc().SizeInBytes;
					uint64 offset = 0;
					descriptorSet->WriteBufferDescriptors(&drawArg.pVertexBuffer, &offset, &size, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
					m_VerticesDescriptorSets[b].PushBack(descriptorSet);
				}

				// Create new descriptor sets for the transforms.
				for (uint32 instanceIndex = 0; instanceIndex < drawArg.InstanceCount; instanceIndex++)
				{
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Transform Buffer Descriptor Set", m_PipelineLayout.Get(), 3, m_DescriptorHeap.Get());
						uint64 size = sizeof(RenderSystem::Instance);
						uint64 offset = instanceIndex * size;
						descriptorSet->WriteBufferDescriptors(&drawArg.pInstanceBuffer, &offset, &size, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
						m_TransformDescriptorSets[b][drawArgIndex].PushBack(descriptorSet);
					}
				}
			}
		}

		m_RenderTargets.Clear();
		for (uint32 d = 0; d < count; d++)
		{
			const DrawArg& drawArg = pDrawArgs[d];

			for (uint32 i = 0; i < drawArg.InstanceCount; i++)
			{
				DrawArgExtensionGroup* extensionGroup = drawArg.ppExtensionGroups[i];

				// We can assume there is only one extension, because this render stage has a DrawArgMask of 2 which is one specific extension.
				uint32 numExtensions = extensionGroup->ExtensionCount;
				for (uint32 e = 0; e < numExtensions; e++)
				{
					uint32 mask = extensionGroup->pExtensionMasks[e];
					if (mask & EntityMaskManager::GetExtensionMask(MeshPaintComponent::Type()))
					{
						DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
						TextureView* textureView = extension.ppTextureViews[0];
						m_RenderTargets.PushBack({ .TextureView = textureView, .DrawArgIndex = d, .InstanceIndex = i });
					}
				}
			}
		}
	}

	void PaintMaskRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool sleeping)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		// Delete old resources.
		TArray<TSharedRef<DeviceChild>>& currentFrameDeviceResourcesToDestroy = m_pDeviceResourcesToDestroy[modFrameIndex];
		if (!currentFrameDeviceResourcesToDestroy.IsEmpty())
		{
			// TODO: This might need to be done. Should need to release the resources, but seems to work if it isn't done too.
			//for (TSharedRef<DeviceChild> pDeviceChild : currentFrameDeviceResourcesToDestroy)
			//{
				//pDeviceChild.Get()->Release();
				//SAFERELEASE(pDeviceChild);
			//}
			currentFrameDeviceResourcesToDestroy.Clear();
		}

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];

		if (m_RenderTargets.IsEmpty())
		{
			return;
		}

		m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);
		for (uint32 t = 0; t < m_RenderTargets.GetSize(); t++)
		{
			RenderTarget	renderTargetDesc	= m_RenderTargets[t];
			uint32			drawArgIndex		= renderTargetDesc.DrawArgIndex;
			uint32			instanceIndex		= renderTargetDesc.InstanceIndex;
			const DrawArg&	drawArg				= m_pDrawArgs[drawArgIndex];
			TextureView*	renderTarget		= renderTargetDesc.TextureView;

			uint32 width	= renderTarget->GetDesc().pTexture->GetDesc().Width;
			uint32 height	= renderTarget->GetDesc().pTexture->GetDesc().Height;

			BeginRenderPassDesc beginRenderPassDesc = {};
			beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
			beginRenderPassDesc.ppRenderTargets = &renderTarget;
			beginRenderPassDesc.RenderTargetCount = 1;
			beginRenderPassDesc.Width = width;
			beginRenderPassDesc.Height = height;
			beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
			beginRenderPassDesc.pClearColors = nullptr;
			beginRenderPassDesc.ClearColorCount = 0;
			beginRenderPassDesc.Offset.x = 0;
			beginRenderPassDesc.Offset.y = 0;

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

			pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

			if (m_PerFrameBufferDescriptorSets.has_value())
			{
				pCommandList->BindDescriptorSetGraphics(m_PerFrameBufferDescriptorSets.value().Get(), m_PipelineLayout.Get(), 0);
			}

			if (m_BrushMaskDescriptorSet.has_value())
			{
				pCommandList->BindDescriptorSetGraphics(m_BrushMaskDescriptorSet.value().Get(), m_PipelineLayout.Get(), 1);
			}

			pCommandList->BindDescriptorSetGraphics(m_VerticesDescriptorSets[modFrameIndex][drawArgIndex].Get(), m_PipelineLayout.Get(), 2);

			if (!m_TransformDescriptorSets.IsEmpty())
			{
				pCommandList->BindDescriptorSetGraphics(m_TransformDescriptorSets[modFrameIndex][drawArgIndex][instanceIndex].Get(), m_PipelineLayout.Get(), 3);
			}

			pCommandList->DrawIndexInstanced(drawArg.IndexCount, 1, 0, 0, 0);

			pCommandList->EndRenderPass();

		}
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
		// PerFrameBuffer
		DescriptorBindingDesc perFrameBufferDesc = {};
		perFrameBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount = 1;
		perFrameBufferDesc.Binding = 0;
		perFrameBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Brush mask texture
		DescriptorBindingDesc brushMaskDesc = {};
		brushMaskDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		brushMaskDesc.DescriptorCount = 1;
		brushMaskDesc.Binding = 0;
		brushMaskDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Draw Args (No Extension, only Vertices)
		DescriptorBindingDesc ssboBindingDesc = {};
		ssboBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		ssboBindingDesc.DescriptorCount = 1;
		ssboBindingDesc.Binding = 0;
		ssboBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Transform
		DescriptorBindingDesc transformBufferDesc = {};
		transformBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		transformBufferDesc.DescriptorCount = 1;
		transformBufferDesc.Binding = 0;
		transformBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { perFrameBufferDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { brushMaskDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { ssboBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc3 = {};
		descriptorSetLayoutDesc3.DescriptorBindings = { transformBufferDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Paint Mask Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2, descriptorSetLayoutDesc3 };

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
		descriptorHeapDesc.DescriptorSetCount = 227;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
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
		colorAttachmentDesc.Format = EFormat::FORMAT_R8G8B8A8_UNORM;
		colorAttachmentDesc.SampleCount = 1;
		colorAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		colorAttachmentDesc.FinalState = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		
		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET };

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

		pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
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