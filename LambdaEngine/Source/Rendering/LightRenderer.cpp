#include "Rendering/LightRenderer.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/RenderAPI.h"

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

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppRenderCommandLists[b]);
			SAFERELEASE(m_ppRenderCommandAllocators[b]);
		}

		SAFEDELETE_ARRAY(m_ppRenderCommandLists);
		SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
	}

	bool LightRenderer::init(uint32 backBufferCount)
	{
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

		return true;
	}

	bool LightRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->pDepthStencilAttachmentDesc == nullptr);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateCommandLists())
		{
			LOG_ERROR("[LightRenderer]: Failed to create render command lists");
			return false;
		}

		if (!CreateRenderPass(&pPreInitDesc->pDepthStencilAttachmentDesc[0]))
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

	void LightRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppTextureViews);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void LightRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
	}

	void LightRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void LightRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (Sleeping)
			return;

		TSharedRef<const TextureView> backBuffer = m_BackBuffers[backBufferIndex];
		uint32 width = backBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height = backBuffer->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets = &backBuffer;
		beginRenderPassDesc.RenderTargetCount = 1;
		beginRenderPassDesc.pDepthStencil = m_DepthStencilBuffer.Get();
		beginRenderPassDesc.Width = width;
		beginRenderPassDesc.Height = height;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = nullptr;
		beginRenderPassDesc.ClearColorCount = 0;
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];
	
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

		pCommandList->BindDescriptorSetGraphics(m_DescriptorSets[0].Get(), m_PipelineLayout.Get(), 0);	// LightsBuffer
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSets[1].Get(), m_PipelineLayout.Get(), 1);	// DrawArgs

		//pCommandList->DrawInstanced(drawCount, drawCount, 0, 0);

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}

	bool LightRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc drawArgsBindingDesc = {};
		drawArgsBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		drawArgsBindingDesc.DescriptorCount = 2;
		drawArgsBindingDesc.Binding = 0;
		drawArgsBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc lightsBindingDesc = {};
		lightsBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		lightsBindingDesc.DescriptorCount = 1;
		lightsBindingDesc.Binding = 0;
		lightsBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { drawArgsBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { lightsBindingDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Light Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };

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

		DescriptorSet* pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Physics Renderer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
		if (pDescriptorSet == nullptr)
		{
			LOG_ERROR("[LightRenderer]: Failed to create DescriptorSet[%d]", 0);
			return false;
		}
		m_DescriptorSets.PushBack(pDescriptorSet);

		pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Physics Renderer Descriptor Set 1", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
		if (pDescriptorSet == nullptr)
		{
			LOG_ERROR("[LightRenderer]: Failed to create DescriptorSet[%d]", 1);
			return false;
		}
		m_DescriptorSets.PushBack(pDescriptorSet);

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
		m_ppRenderCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppRenderCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Light Renderer Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Light Renderer Graphics Command List " + std::to_string(b);
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppRenderCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

			if (!m_ppRenderCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool LightRenderer::CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc)
	{
		RenderPassAttachmentDesc depthAttachmentDesc = {};
		depthAttachmentDesc.Format = EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthAttachmentDesc.SampleCount = 1;
		depthAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		depthAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		depthAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		depthAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		depthAttachmentDesc.InitialState = pBackBufferAttachmentDesc->InitialState;
		depthAttachmentDesc.FinalState = pBackBufferAttachmentDesc->FinalState;

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
		pipelineStateDesc.DepthStencilState.DepthTestEnable = false;
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

}