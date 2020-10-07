#include "Rendering/LightRenderer.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"

namespace LambdaEngine
{

	LightRenderer* LightRenderer::s_pInstance = nullptr;
	
	LambdaEngine::LightRenderer::LightRenderer()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;
	}

	LambdaEngine::LightRenderer::~LightRenderer()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;

		SAFEDELETE(m_PushConstant.pData);

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppGraphicCommandLists[b]);
			SAFERELEASE(m_ppGraphicCommandAllocators[b]);
		}

		SAFEDELETE_ARRAY(m_ppGraphicCommandLists);
		SAFEDELETE_ARRAY(m_ppGraphicCommandAllocators);
	}

	bool LightRenderer::init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;

		m_UsingMeshShader = EngineConfig::GetBoolProperty("MeshShadersEnabled");

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[LightRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[LightRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[LightRenderer]: Failed to create Shaders");
			return false;
		}

		// Allocate pushConstant data
		m_PushConstant.pData = DBG_NEW byte[DRAW_ITERATION_PUSH_CONSTANTS_SIZE];
		m_PushConstant.DataSize = DRAW_ITERATION_PUSH_CONSTANTS_SIZE;
		m_PushConstant.Offset = 0U;
		m_PushConstant.MaxDataSize = DRAW_ITERATION_PUSH_CONSTANTS_SIZE;

		return true;
	}

	bool LightRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->pDepthStencilAttachmentDesc != nullptr);

		if (!CreateCommandLists())
		{
			LOG_ERROR("[LightRenderer]: Failed to create render command lists");
			return false;
		}

		if (!CreateRenderPass(pPreInitDesc->pDepthStencilAttachmentDesc))
		{
			LOG_ERROR("[LightRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[LightRenderer]: Failed to create PipelineState");
			return false;
		}

		return true;
	}

	void LightRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void LightRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void LightRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == SCENE_POINT_SHADOWMAPS)
		{
			constexpr uint32 CUBE_FACE_COUNT = 6U;

			m_PointLightCount = imageCount;

			m_PointLFaceViews.Clear();
			m_PointLFaceViews.Resize(imageCount * CUBE_FACE_COUNT);
			for (uint32 c = 0; c < imageCount; c++)
			{
				for (uint32 f = 0; f < CUBE_FACE_COUNT; f++)
				{
					m_PointLFaceViews[f + c * CUBE_FACE_COUNT] = MakeSharedRef(ppPerSubImageTextureViews[f + c * CUBE_FACE_COUNT]);
				}
			}
		}
	}

	void LightRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == SCENE_LIGHTS_BUFFER)
		{
			DescriptorSet* ds;

			m_LightDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Light Renderer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
			if (m_LightDescriptorSet != nullptr)
			{
				m_LightDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					0,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[LightRenderer]: Failed to update DescriptorSet[%d]", 0);
			}
		}
	}

	void LightRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void LightRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		if (resourceName == SCENE_DRAW_ARGS)
		{
			if (count > 0U && pDrawArgs != nullptr)
			{
				m_pDrawArgs = pDrawArgs;
				m_DrawCount = count;

				m_DrawArgsDescriptorSets.Clear();
				m_DrawArgsDescriptorSets.Resize(m_DrawCount);
				
				// Create DrawArgs Descriptors
				// TODO: Get descriptors instead of reacreating them
				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					m_DrawArgsDescriptorSets[d] = GetDrawArgsDescriptorSet("Light Renderer Descriptor Set 1", 1);

					if (m_DrawArgsDescriptorSets[d] != nullptr)
					{
						Buffer* ppBuffers[2] = { m_pDrawArgs[d].pVertexBuffer, m_pDrawArgs[d].pInstanceBuffer };
						uint64 pOffsets[2]	 = { 0, 0 };
						uint64 pSizes[2]	 = { m_pDrawArgs[d].pVertexBuffer->GetDesc().SizeInBytes, m_pDrawArgs[d].pInstanceBuffer->GetDesc().SizeInBytes };

						m_DrawArgsDescriptorSets[d]->WriteBufferDescriptors(
							ppBuffers,
							pOffsets,
							pSizes,
							0,
							2,
							EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
						);
					}
				}
			}
			else
			{
				LOG_ERROR("[LightRenderer]: Failed to update descriptors for drawArgs");
			}
		}
	}

	void LightRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (Sleeping)
			return;

		CommandList* pCommandList = m_ppGraphicCommandLists[modFrameIndex];
	
		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));
		pCommandList->BindDescriptorSetGraphics(m_LightDescriptorSet.Get(), m_PipelineLayout.Get(), 0);	// LightsBuffer
		
		constexpr uint32 CUBE_FACE_COUNT = 6U;
		for (uint32 c = 0; c < m_PointLightCount; c++)
		{
			for (uint32 f = 0; f < CUBE_FACE_COUNT; f++)
			{
				auto pFaceView = m_PointLFaceViews[c * CUBE_FACE_COUNT + f];

				uint32 width = pFaceView->GetDesc().pTexture->GetDesc().Width;
				uint32 height = pFaceView->GetDesc().pTexture->GetDesc().Height;

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


				ClearColorDesc clearColorDesc = {};
				clearColorDesc.Depth = 1.0;
				clearColorDesc.Stencil = 0U;

				BeginRenderPassDesc beginRenderPassDesc = {};
				beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
				beginRenderPassDesc.ppRenderTargets = nullptr;
				beginRenderPassDesc.RenderTargetCount = 0;
				beginRenderPassDesc.pDepthStencil = pFaceView.Get();
				beginRenderPassDesc.Width = width;
				beginRenderPassDesc.Height = height;
				beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
				beginRenderPassDesc.pClearColors = &clearColorDesc;
				beginRenderPassDesc.ClearColorCount = 1;
				beginRenderPassDesc.Offset.x = 0;
				beginRenderPassDesc.Offset.y = 0;
				
				uint32 iteration = c * CUBE_FACE_COUNT + f;

				memcpy(m_PushConstant.pData, &iteration, sizeof(uint32));
				pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, m_PushConstant.pData, m_PushConstant.DataSize, m_PushConstant.Offset);
				pCommandList->BeginRenderPass(&beginRenderPassDesc);

				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					const DrawArg& drawArg = m_pDrawArgs[d];
					pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

					auto* descriptorSet = m_DrawArgsDescriptorSets[d].Get();

					pCommandList->BindDescriptorSetGraphics(descriptorSet, m_PipelineLayout.Get(), 1);

					pCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
				}
				
				pCommandList->EndRenderPass();
			}
		}
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}

	bool LightRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc verticesBindingDesc = {};
		verticesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		verticesBindingDesc.DescriptorCount = 1;
		verticesBindingDesc.Binding = 0;
		verticesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		
		DescriptorBindingDesc instanceBindingDesc = {};
		instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc.DescriptorCount = 1;
		instanceBindingDesc.Binding = 1;
		instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc lightsBindingDesc = {};
		lightsBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		lightsBindingDesc.DescriptorCount = 1;
		lightsBindingDesc.Binding = 0;
		lightsBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { lightsBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc };

		ConstantRangeDesc constantRangeDesc = {};
		constantRangeDesc.OffsetInBytes = 0U;
		constantRangeDesc.SizeInBytes = DRAW_ITERATION_PUSH_CONSTANTS_SIZE;
		constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Light Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };
		pipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool LightRenderer::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 0;
		descriptorCountDesc.ConstantBufferDescriptorCount = 0;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 3;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Physics Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 64;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_LightDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Light Renderer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
		if (m_LightDescriptorSet == nullptr)
		{
			LOG_ERROR("[LightRenderer]: Failed to create DescriptorSet[%d]", 0);
			return false;
		}

		return true;
	}

	bool LightRenderer::CreateShaders()
	{
		m_VertexShaderPointGUID = ResourceManager::LoadShaderFromFile("/ShadowMap/PointLShadowMap.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderPointGUID = ResourceManager::LoadShaderFromFile("/ShadowMap/PointLShadowMap.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderPointGUID != GUID_NONE && m_PixelShaderPointGUID != GUID_NONE;
	}

	bool LightRenderer::CreateCommandLists()
	{
		m_ppGraphicCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppGraphicCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppGraphicCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Light Renderer Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppGraphicCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Light Renderer Graphics Command List " + std::to_string(b);
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppGraphicCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppGraphicCommandAllocators[b], &commandListDesc);

			if (!m_ppGraphicCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool LightRenderer::CreateRenderPass(RenderPassAttachmentDesc* pDepthStencilAttachmentDesc)
	{
		RenderPassAttachmentDesc depthAttachmentDesc = {};
		depthAttachmentDesc.Format = EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthAttachmentDesc.SampleCount = 1;
		depthAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_CLEAR;
		depthAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		depthAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		depthAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		depthAttachmentDesc.InitialState = pDepthStencilAttachmentDesc->InitialState;
		depthAttachmentDesc.FinalState = pDepthStencilAttachmentDesc->FinalState;
		
		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = {};
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass = EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass = 0;
		subpassDependencyDesc.SrcAccessMask = 0;
		subpassDependencyDesc.DstAccessMask = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName = "Light Renderer Render Pass";
		renderPassDesc.Attachments = { depthAttachmentDesc };
		renderPassDesc.Subpasses = { subpassDesc };
		renderPassDesc.SubpassDependencies = { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool LightRenderer::CreatePipelineState()
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName = "Point Light Renderer Pipeline State";
		pipelineStateDesc.RenderPass = m_RenderPass;
		pipelineStateDesc.PipelineLayout = m_PipelineLayout;

		pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
		pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_BACK;

		pipelineStateDesc.DepthStencilState = {};
		pipelineStateDesc.DepthStencilState.DepthTestEnable = true;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable = true;

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

		pipelineStateDesc.VertexShader.ShaderGUID = m_VertexShaderPointGUID;
		pipelineStateDesc.PixelShader.ShaderGUID = m_PixelShaderPointGUID;

		m_PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		return true;
	}

	DescriptorSet* LightRenderer::GetDrawArgsDescriptorSet(const String& debugname, uint32 descriptorLayoutIndex)
	{
		DescriptorSet* ds;
			
		ds = RenderAPI::GetDevice()->CreateDescriptorSet("Light Renderer Descriptor Set 1", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
		if (ds == nullptr)
		{
			LOG_ERROR("[LightRenderer]: Failed to create DescriptorSet[%d]", 1);
			return nullptr;
		}

		return 	ds;
		return nullptr;
	}

}