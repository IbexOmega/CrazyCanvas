#include "Rendering/AARenderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/CommandQueue.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	AARenderer::AARenderer()
		: CustomRenderer()
		, m_AAMode(EAAMode::AAMODE_TAA)
		, m_TAAHistory()
		, m_TAAHistoryViews()
		, m_CommandLists()
		, m_CommandAllocators()
		, m_TAAState()
		, m_TAALayout()
		, m_FXAAState()
		, m_FXAALayout()
		, m_DescriptorHeap()
		, m_TAATextureDescriptorSets()
		, m_FXAADescriptorSets()
		, m_Width(0)
		, m_Height(0) 
	{
		s_pInstance = this;
	}

	AARenderer::~AARenderer()
	{
		s_pInstance = nullptr;
		EventQueue::UnregisterEventHandler(this, &AARenderer::OnWindowResized);
	}

	bool AARenderer::Init()
	{
		{
			DescriptorBindingDesc perFrameBinding = { };
			perFrameBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
			perFrameBinding.DescriptorCount	= 1;
			perFrameBinding.Binding			= 0;
			perFrameBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorSetLayoutDesc bufferLayoutDesc = { };
			bufferLayoutDesc.DescriptorBindings =
			{
				perFrameBinding,
			};

			DescriptorBindingDesc intermediateBinding = { };
			intermediateBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			intermediateBinding.DescriptorCount	= 1;
			intermediateBinding.Binding			= 0;
			intermediateBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorBindingDesc velocityBinding = { };
			velocityBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			velocityBinding.DescriptorCount	= 1;
			velocityBinding.Binding			= 1;
			velocityBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorBindingDesc depthBinding = { };
			depthBinding.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			depthBinding.DescriptorCount	= 1;
			depthBinding.Binding			= 2;
			depthBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorBindingDesc historyBinding = { };
			historyBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			historyBinding.DescriptorCount	= 1;
			historyBinding.Binding			= 3;
			historyBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorSetLayoutDesc textureLayoutDesc = { };
			textureLayoutDesc.DescriptorBindings =
			{
				intermediateBinding,
				velocityBinding,
				depthBinding,
				historyBinding
			};

			PipelineLayoutDesc taaLayoutDesc;
			taaLayoutDesc.DebugName = "TAA Layout";
			taaLayoutDesc.DescriptorSetLayouts =
			{
				bufferLayoutDesc,
				textureLayoutDesc
			};

			m_TAALayout = RenderAPI::GetDevice()->CreatePipelineLayout(&taaLayoutDesc);
			if (!m_TAALayout)
			{
				LOG_ERROR("[AARenderer]: Failed to create TAA Layout");
				DEBUGBREAK();
				return false;
			}
		}

		{
			DescriptorBindingDesc intermediateBinding = { };
			intermediateBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			intermediateBinding.DescriptorCount	= 1;
			intermediateBinding.Binding			= 0;
			intermediateBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				intermediateBinding
			};

			PipelineLayoutDesc fxaaLayoutDesc;
			fxaaLayoutDesc.DebugName			= "FXAA Layout";
			fxaaLayoutDesc.DescriptorSetLayouts	=
			{
				descriptorSetLayoutDesc
			};

			m_FXAALayout = RenderAPI::GetDevice()->CreatePipelineLayout(&fxaaLayoutDesc);
			if (!m_FXAALayout)
			{
				LOG_ERROR("[AARenderer]: Failed to create FXAA Layout");
				DEBUGBREAK();
				return false;
			}
		}

		if (!InitHistoryTextures())
		{
			return false;
		}

		EventQueue::RegisterEventHandler(this, &AARenderer::OnWindowResized);
		return true;
	}

	bool AARenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		const RenderPassAttachmentDesc& backBufferAttachmentDesc = pPreInitDesc->pColorAttachmentDesc[0];
		const uint32 NUM_FRAMES = pPreInitDesc->BackBufferCount;

		// Create renderpass
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= (backBufferAttachmentDesc.InitialState != ETextureState::TEXTURE_STATE_UNKNOWN) ? ELoadOp::LOAD_OP_LOAD : ELoadOp::LOAD_OP_CLEAR;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= backBufferAttachmentDesc.InitialState;
		colorAttachmentDesc.FinalState		= backBufferAttachmentDesc.FinalState;

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
		renderPassDesc.DebugName			= "FXAA RenderPass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);
		if (!m_RenderPass)
		{
			LOG_ERROR("[AARenderer] Failed to create FXAA RenderPass");
			DEBUGBREAK();
			return false;
		}

		renderPassDesc.DebugName			= "TAA RenderPass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_TAARenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);
		if (!m_TAARenderPass)
		{
			LOG_ERROR("[AARenderer] Failed to create TAA RenderPass");
			DEBUGBREAK();
			return false;
		}

		// Create Descriptor Heap and allocate descriptorsets
		{
			const uint32 NUM_SAMPLERS	= 4;
			const uint32 NUM_BUFFERS	= 1;
			const uint32 NUM_DESCRIPTOR_SETS	= 3;
			const uint32 NEEDED_DESCRIPTOR_SETS = NUM_FRAMES * NUM_DESCRIPTOR_SETS;
			const uint32 NEEDED_SAMPLER_COUNT	= NUM_FRAMES * NUM_SAMPLERS;
			const uint32 NEEDED_BUFFER_COUNT	= NUM_FRAMES * NUM_BUFFERS;

			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= NEEDED_SAMPLER_COUNT;
			descriptorCountDesc.ConstantBufferDescriptorCount			= NEEDED_BUFFER_COUNT;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName			= "AA DescriptorHeap";
			descriptorHeapDesc.DescriptorSetCount	= NEEDED_DESCRIPTOR_SETS;
			descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

			m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
			if (!m_DescriptorHeap)
			{
				LOG_ERROR("[AARenderer] Failed to create AA DescriptorHeap");
				DEBUGBREAK();
				return false;
			}

			// Create frame resources
			for (uint32 i = 0; i < NUM_FRAMES; i++)
			{
				String name = "FXAA DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> fxaaSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_FXAALayout.Get(),
					0,
					m_DescriptorHeap.Get());
				if (!fxaaSet)
				{
					LOG_ERROR("[AARenderer] Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_FXAADescriptorSets.EmplaceBack(fxaaSet);
				}

				name = "TAA Buffer DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> taaBufferSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_TAALayout.Get(),
					0,
					m_DescriptorHeap.Get());
				if (!taaBufferSet)
				{
					LOG_ERROR("[AARenderer] Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_TAABufferDescriptorSets.EmplaceBack(taaBufferSet);
				}

				name = "TAA Texture DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> taaTextureSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_TAALayout.Get(),
					1,
					m_DescriptorHeap.Get());
				if (!taaTextureSet)
				{
					LOG_ERROR("[AARenderer] Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_TAATextureDescriptorSets.EmplaceBack(taaTextureSet);
				}
			}
		}

		// Create commandlists
		{
			for (uint32 i = 0; i < NUM_FRAMES; i++)
			{
				String name = "AA CommandAllocator[" + std::to_string(i) + "]";
				TSharedRef<CommandAllocator> commandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator(
					name,
					ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
				if (!commandAllocator)
				{
					LOG_ERROR("[AARenderer] Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_CommandAllocators.EmplaceBack(commandAllocator);
				}

				name = "AA CommandList[" + std::to_string(i) + "]";
				CommandListDesc commandListDesc = {};
				commandListDesc.DebugName		= name;
				commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
				commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

				TSharedRef<CommandList> commandList = RenderAPI::GetDevice()->CreateCommandList(
					commandAllocator.Get(),
					&commandListDesc);
				if (!commandList)
				{
					LOG_ERROR("[AARenderer] Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_CommandLists.EmplaceBack(commandList);
				}
			}
		}

		GUID_Lambda FullscreenShader = ResourceManager::LoadShaderFromFile(
			"Helpers/FullscreenQuad.vert",
			FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER,
			EShaderLang::SHADER_LANG_GLSL,
			"main");
		if (FullscreenShader == GUID_NONE)
		{
			DEBUGBREAK();
			return false;
		}

		// Default BlendAttachmentStateDesc
		BlendAttachmentStateDesc blendAttachmentState = { };

		{
			GUID_Lambda taaShader = ResourceManager::LoadShaderFromFile(
				"PostProcess/TAA_accumulation.frag",
				FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (taaShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedGraphicsPipelineStateDesc taaStateDesc;
			taaStateDesc.DebugName		= "TAA State";
			taaStateDesc.RenderPass		= m_RenderPass;
			taaStateDesc.PipelineLayout = m_TAALayout;
			taaStateDesc.BlendState.BlendAttachmentStates = { blendAttachmentState };
			taaStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;
			taaStateDesc.DepthStencilState.DepthTestEnable			= false;
			taaStateDesc.DepthStencilState.DepthBoundsTestEnable	= false;
			taaStateDesc.VertexShader.ShaderGUID	= FullscreenShader;
			taaStateDesc.PixelShader.ShaderGUID		= taaShader;

			m_TAAState = PipelineStateManager::CreateGraphicsPipelineState(&taaStateDesc);
			if (m_TAAState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		{
			GUID_Lambda fxaaShader = ResourceManager::LoadShaderFromFile(
				"PostProcess/FXAA.frag",
				FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (fxaaShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedGraphicsPipelineStateDesc fxaaStateDesc;
			fxaaStateDesc.DebugName			= "FXAA State";
			fxaaStateDesc.RenderPass		= m_RenderPass;
			fxaaStateDesc.PipelineLayout	= m_FXAALayout;
			fxaaStateDesc.BlendState.BlendAttachmentStates = { blendAttachmentState };
			fxaaStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;
			fxaaStateDesc.DepthStencilState.DepthTestEnable			= false;
			fxaaStateDesc.DepthStencilState.DepthBoundsTestEnable	= false;
			fxaaStateDesc.VertexShader.ShaderGUID	= FullscreenShader;
			fxaaStateDesc.PixelShader.ShaderGUID	= fxaaShader;

			m_FXAAState = PipelineStateManager::CreateGraphicsPipelineState(&fxaaStateDesc);
			if (m_FXAAState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		return true;
	}

	void AARenderer::UpdateTextureResource(
		const String& resourceName,
		const TextureView* const* ppPerImageTextureViews,
		const TextureView* const* ppPerSubImageTextureViews,
		const Sampler* const* ppPerImageSamplers,
		uint32 imageCount,
		uint32 subImageCount,
		bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "INTERMEDIATE_OUTPUT_IMAGE")
		{
			m_IntermediateOutputView	= MakeSharedRef(ppPerImageTextureViews[0]);
			m_IntermediateOutput		= MakeSharedRef(m_IntermediateOutputView->GetTexture());

			WriteDescriptorSets();
		}
		else if (resourceName == "G_BUFFER_DEPTH_STENCIL")
		{
			m_DepthView	= MakeSharedRef(ppPerImageTextureViews[0]);
			m_Depth		= MakeSharedRef(m_DepthView->GetTexture());

			WriteDescriptorSets();
		}
		else if (resourceName == "G_BUFFER_VELOCITY")
		{
			m_VelocityView	= MakeSharedRef(ppPerImageTextureViews[0]);
			m_Velocity		= MakeSharedRef(m_VelocityView->GetTexture());

			WriteDescriptorSets();
		}
		else if (resourceName == "BACK_BUFFER_TEXTURE")
		{
			for (uint32 i = 0; i < imageCount; i++)
			{
				TSharedRef<TextureView>	backbufferView	= MakeSharedRef(const_cast<TextureView*>(ppPerImageTextureViews[i]));
				TSharedRef<Texture>		backbuffer		= MakeSharedRef(backbufferView->GetTexture());
				m_BackBuffers.PushBack(backbuffer);
				m_BackBufferViews.PushBack(backbufferView);
			}
		}
	}

	void AARenderer::UpdateBufferResource(
		const String& resourceName, 
		const Buffer* const* ppBuffers, 
		uint64* pOffsets, 
		uint64* pSizesInBytes, 
		uint32 count, 
		bool backBufferBound)
	{
		if (resourceName == "PER_FRAME_BUFFER")
		{
			m_PerFrameBuffer = MakeSharedRef(ppBuffers[0]);
			WriteDescriptorSets();
		}
	}

	void AARenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (sleeping)
		{
			return;
		}

		TSharedRef<CommandAllocator>	commandAllocator	= m_CommandAllocators[modFrameIndex];
		TSharedRef<CommandList>			commandList			= m_CommandLists[modFrameIndex];
		TSharedRef<const Texture>		backBuffer		= m_BackBuffers[backBufferIndex];
		TSharedRef<const TextureView>	backBufferView	= m_BackBufferViews[backBufferIndex];
		const uint32 width	= backBuffer->GetDesc().Width;
		const uint32 height	= backBuffer->GetDesc().Height;

		commandAllocator->Reset();
		commandList->Begin(nullptr);

		if (m_AAMode == EAAMode::AAMODE_NONE)
		{
			Texture* pBackBuffer = const_cast<Texture*>(backBuffer.Get());
			commandList->BlitTexture(
				m_IntermediateOutput.Get(), 
				ETextureState::TEXTURE_STATE_COPY_SRC, 
				pBackBuffer, 
				ETextureState::TEXTURE_STATE_COPY_DST, 
				EFilterType::FILTER_TYPE_NEAREST);
		}
		else
		{
			ClearColorDesc clearColorDesc;
			clearColorDesc.Color[0] = 0.0f;
			clearColorDesc.Color[1] = 0.0f;
			clearColorDesc.Color[2] = 0.0f;
			clearColorDesc.Color[3] = 1.0f;

			BeginRenderPassDesc beginRenderPassDesc;
			beginRenderPassDesc.pRenderPass			= m_RenderPass.Get();
			beginRenderPassDesc.ppRenderTargets		= &backBufferView;
			beginRenderPassDesc.RenderTargetCount	= 1;
			beginRenderPassDesc.pDepthStencil		= nullptr;
			beginRenderPassDesc.Width				= width;
			beginRenderPassDesc.Height				= height;
			beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
			beginRenderPassDesc.pClearColors		= &clearColorDesc;
			beginRenderPassDesc.ClearColorCount		= 1;
			beginRenderPassDesc.Offset.x			= 0;
			beginRenderPassDesc.Offset.y			= 0;

			commandList->BeginRenderPass(&beginRenderPassDesc);
			
			Viewport viewport;
			viewport.Height		= height;
			viewport.Width		= width;
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;
			viewport.MaxDepth	= 1.0f;
			viewport.MaxDepth	= 0.0f;
			commandList->SetViewports(&viewport, 0, 1);

			ScissorRect scissorRect;
			scissorRect.Height	= height;
			scissorRect.Width	= width;
			scissorRect.x		= 0.0f;
			scissorRect.y		= 0.0f;
			commandList->SetScissorRects(&scissorRect, 0, 1);

			if (m_AAMode == EAAMode::AAMODE_TAA)
			{
				PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(m_TAAState);
				commandList->BindGraphicsPipeline(pPipelineState);
				commandList->BindDescriptorSetGraphics(m_TAABufferDescriptorSets[modFrameIndex].Get(), m_TAALayout.Get(), 0);
				commandList->BindDescriptorSetGraphics(m_TAATextureDescriptorSets[modFrameIndex].Get(), m_TAALayout.Get(), 1);
			}
			else if (m_AAMode == EAAMode::AAMODE_FXAA)
			{
				PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(m_FXAAState);
				commandList->BindGraphicsPipeline(pPipelineState);
				commandList->BindDescriptorSetGraphics(m_FXAADescriptorSets[modFrameIndex].Get(), m_FXAALayout.Get(), 0);
			}

			commandList->DrawInstanced(3, 1, 0, 0);
			commandList->EndRenderPass();
		}

		commandList->End();
		(*ppFirstExecutionStage) = commandList.Get();
	}

	void AARenderer::WriteDescriptorSets()
	{
		const uint32 numDescriptorSets = m_TAATextureDescriptorSets.GetSize();
		for (uint32 i = 0; i < numDescriptorSets; i++)
		{
			TSharedRef<DescriptorSet>& taaBufferSet = m_TAABufferDescriptorSets[i];
			if (m_PerFrameBuffer)
			{
				uint64 offset	= 0;
				uint64 size		= m_PerFrameBuffer->GetDesc().SizeInBytes;
				taaBufferSet->WriteBufferDescriptors(
					&m_PerFrameBuffer, 
					&offset, 
					&size, 
					0, 1, 
					EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
			}

			TSharedRef<DescriptorSet>& taaTextureSet = m_TAATextureDescriptorSets[i];
			if (m_IntermediateOutputView)
			{
				taaTextureSet->WriteTextureDescriptors(
					&m_IntermediateOutputView,
					Sampler::GetLinearSamplerToBind(),
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					0, 1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					false);
			}
			
			if (m_TAAHistoryViews.GetSize() > i)
			{
				TSharedRef<TextureView>& taaHistoryView = m_TAAHistoryViews[i];
				if (taaHistoryView)
				{
					taaTextureSet->WriteTextureDescriptors(
						&taaHistoryView, 
						Sampler::GetLinearSamplerToBind(),
						ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
						1, 1, 
						EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, 
						false);
				}
			}

			if (m_VelocityView)
			{
				taaTextureSet->WriteTextureDescriptors(
					&m_VelocityView,
					Sampler::GetLinearSamplerToBind(),
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					2, 1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					false);
			}

			if (m_DepthView)
			{
				taaTextureSet->WriteTextureDescriptors(
					&m_DepthView,
					Sampler::GetLinearSamplerToBind(),
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					3, 1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					false);
			}
		}
	}
	
	bool AARenderer::InitHistoryTextures()
	{
		if (m_Width == 0 || m_Height == 0)
		{
			TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
			m_Width		= mainWindow->GetWidth();
			m_Height	= mainWindow->GetHeight();
		}

		TextureDesc historyBuffDesc;
		historyBuffDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		historyBuffDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
		historyBuffDesc.Flags		= FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET;
		historyBuffDesc.Width		= m_Width;
		historyBuffDesc.Height		= m_Height;
		historyBuffDesc.ArrayCount	= 1;
		historyBuffDesc.Depth		= 1;
		historyBuffDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		historyBuffDesc.Miplevels	= 1;
		historyBuffDesc.SampleCount = 1;

		TextureViewDesc historyBuffViewDesc;
		historyBuffViewDesc.ArrayCount		= 1;
		historyBuffViewDesc.ArrayIndex		= 0;
		historyBuffViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		historyBuffViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE;
		historyBuffViewDesc.Format			= historyBuffDesc.Format;
		historyBuffViewDesc.Miplevel		= 0;
		historyBuffViewDesc.MiplevelCount	= 1;

		TSharedRef<CommandAllocator>&	commandAllocator	= m_CommandAllocators[0];
		TSharedRef<CommandList>&		commandList			= m_CommandLists[0];

		commandAllocator->Reset();
		commandList->Begin(nullptr);

		for (uint32 i = 0; i < 2; i++)
		{
			// Create texture
			historyBuffDesc.DebugName = "TAA History Buffer" + std::to_string(i);

			TSharedRef<Texture> taaHistory = RenderAPI::GetDevice()->CreateTexture(&historyBuffDesc);
			if (!taaHistory)
			{
				DEBUGBREAK();
				return false;
			}

			commandList->TransitionBarrier(
				taaHistory.Get(), 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, 
				0, 0, 
				ETextureState::TEXTURE_STATE_UNKNOWN,
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);

			m_TAAHistory.EmplaceBack(taaHistory);

			// Create textureview
			historyBuffViewDesc.DebugName	= "TAA History Buffer View" + std::to_string(i);;
			historyBuffViewDesc.pTexture	= taaHistory.Get();

			TSharedRef<TextureView> taaHistoryView = RenderAPI::GetDevice()->CreateTextureView(&historyBuffViewDesc);
			if (!taaHistoryView)
			{
				DEBUGBREAK();
				return false;
			}

			m_TAAHistoryViews.EmplaceBack(taaHistoryView);
		}

		commandList->End();

		RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&commandList, 1, 0, nullptr, 0, nullptr, 0);
		RenderAPI::GetGraphicsQueue()->Flush();

		return true;
	}

	bool AARenderer::OnWindowResized(const WindowResizedEvent& windowResizedEvent)
	{
		// Wait for graphicsqueue
		RenderAPI::GetGraphicsQueue()->Flush();

		// Resize buffers
		m_Width		= windowResizedEvent.Width;
		m_Height	= windowResizedEvent.Height;

		InitHistoryTextures();
		WriteDescriptorSets();

		return true;
	}
}