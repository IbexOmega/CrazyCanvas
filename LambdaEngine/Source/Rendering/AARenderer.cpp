#include "Rendering/AARenderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	constexpr uint32 MAX_SPECULAR_MIPS = 10;
	constexpr uint32 DIFFUSE_RESOURCE_COUNT = 1;
	constexpr uint32 NUM_FRAMES = 3;

	AARenderer::AARenderer()
		: CustomRenderer()
		, m_TAAHistory()
		, m_TAAHistoryViews()
		, m_CommandLists()
		, m_CommandAllocators()
		, m_TAAState()
		, m_TAALayout()
		, m_FXAAState()
		, m_FXAALayout()
		, m_DescriptorHeap()
		, m_TAADescriptorSets()
		, m_FXAADescriptorSets()
	{
		s_pInstance = this;
	}

	AARenderer::~AARenderer()
	{
		s_pInstance = nullptr;
	}

	bool AARenderer::Init()
	{
		{
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
			depthBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			depthBinding.DescriptorCount	= 1;
			depthBinding.Binding			= 2;
			depthBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorBindingDesc historyBinding = { };
			historyBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			historyBinding.DescriptorCount	= 1;
			historyBinding.Binding			= 3;
			historyBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				intermediateBinding,
				velocityBinding,
				depthBinding,
				historyBinding
			};

			PipelineLayoutDesc specularLayoutDesc;
			specularLayoutDesc.DebugName = "TAA Layout";
			specularLayoutDesc.DescriptorSetLayouts =
			{
				descriptorSetLayoutDesc
			};

			m_TAALayout = RenderAPI::GetDevice()->CreatePipelineLayout(&specularLayoutDesc);
			if (!m_TAALayout)
			{
				LOG_ERROR("[AARenderer]: Failed to create TAA Layout");
				DEBUGBREAK();
				return false;
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
			taaStateDesc.PipelineLayout	= m_TAALayout;
			taaStateDesc.RasterizerState.CullMode	= ECullMode::CULL_MODE_NONE;
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

			PipelineLayoutDesc diffuseLayoutDesc;
			diffuseLayoutDesc.DebugName				= "FXAA Layout";
			diffuseLayoutDesc.DescriptorSetLayouts	=
			{
				descriptorSetLayoutDesc
			};

			m_FXAALayout = RenderAPI::GetDevice()->CreatePipelineLayout(&diffuseLayoutDesc);
			if (!m_FXAALayout)
			{
				LOG_ERROR("[AARenderer]: Failed to create FXAA Layout");
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

			ManagedGraphicsPipelineStateDesc taaStateDesc;
			taaStateDesc.DebugName		= "FXAA State";
			taaStateDesc.RenderPass		= m_RenderPass;
			taaStateDesc.PipelineLayout	= m_FXAALayout;
			taaStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;
			taaStateDesc.DepthStencilState.DepthTestEnable			= false;
			taaStateDesc.DepthStencilState.DepthBoundsTestEnable	= false;
			taaStateDesc.VertexShader.ShaderGUID	= FullscreenShader;
			taaStateDesc.PixelShader.ShaderGUID		= fxaaShader;

			m_FXAAState = PipelineStateManager::CreateGraphicsPipelineState(&taaStateDesc);
			if (m_FXAAState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		return true;
	}

	bool AARenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		const RenderPassAttachmentDesc& backBufferAttachmentDesc = pPreInitDesc->pColorAttachmentDesc[0];

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
		renderPassDesc.DebugName			= "AA Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);
		if (m_RenderPass)
		{
			LOG_ERROR("[AARenderer: Failed to create renderpass]");
			DEBUGBREAK();
			return false;
		}

		// Create Descriptor Heap and allocate descriptorsets
		{
			const uint32 NUM_FRAMES		= pPreInitDesc->BackBufferCount;
			const uint32 NUM_SAMPLERS	= 4;
			const uint32 NEEDED_RESOURCE_COUNT = NUM_FRAMES * NUM_SAMPLERS;

			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.TextureCombinedSamplerDescriptorCount = NEEDED_RESOURCE_COUNT;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName			= "AA DescriptorHeap";
			descriptorHeapDesc.DescriptorSetCount	= NEEDED_RESOURCE_COUNT;
			descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

			m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
			if (!m_DescriptorHeap)
			{
				LOG_ERROR("Failed to create AA DescriptorHeap");
				return false;
			}

			// Create frame resources
			for (uint32 i = 0; i < NUM_FRAMES; i++)
			{
				String name = "FXAA DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> taaSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_FXAALayout.Get(),
					0,
					m_DescriptorHeap.Get());
				if (!taaSet)
				{
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_FXAADescriptorSets.EmplaceBack(taaSet);
				}

				String name = "TAA DescriptorSet[" + std::to_string(i) + "]";
				TSharedRef<DescriptorSet> taaSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					name,
					m_TAALayout.Get(),
					0,
					m_DescriptorHeap.Get());
				if (!taaSet)
				{
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_TAADescriptorSets.EmplaceBack(taaSet);
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
					LOG_ERROR("Failed to create '%s'", name.c_str());
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
					LOG_ERROR("Failed to create '%s'", name.c_str());
					DEBUGBREAK();
					return false;
				}
				else
				{
					m_CommandLists.EmplaceBack(commandList);
				}
			}
		}


		return false;
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

		if (resourceName == "IMMEDIATE_OUTPUT")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pEnvironmentMapView = ppPerImageTextureViews[0];
			m_pEnvironmentMap = m_pEnvironmentMapView->GetDesc().pTexture;
			m_NeedsUpdate = true;
		}
		else if (resourceName == "VELOCITY_BUFFER")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pGlobalSpecularView = ppPerImageTextureViews[0];
			m_pGlobalSpecular = m_pGlobalSpecularView->GetDesc().pTexture;
			VALIDATE(m_pGlobalSpecular->GetDesc().Miplevels < MAX_SPECULAR_MIPS);

			m_GlobalSpecularWriteViews.Reserve(subImageCount);
			for (uint32 i = 0; i < subImageCount; i++)
			{
				m_GlobalSpecularWriteViews.EmplaceBack(ppPerSubImageTextureViews[i]);
			}

			m_NeedsUpdate = true;
		}
		else if (resourceName == "DEPTH_BUFFER")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pGlobalSpecularView = ppPerImageTextureViews[0];
			m_pGlobalSpecular = m_pGlobalSpecularView->GetDesc().pTexture;
			VALIDATE(m_pGlobalSpecular->GetDesc().Miplevels < MAX_SPECULAR_MIPS);

			m_GlobalSpecularWriteViews.Reserve(subImageCount);
			for (uint32 i = 0; i < subImageCount; i++)
			{
				m_GlobalSpecularWriteViews.EmplaceBack(ppPerSubImageTextureViews[i]);
			}

			m_NeedsUpdate = true;
		}
		else if (resourceName == "GLOBAL_DIFFUSE_PROBE")
		{
			VALIDATE(imageCount > 0);
			VALIDATE(ppPerImageTextureViews[0] != nullptr);

			m_pGlobalDiffuseView = ppPerImageTextureViews[0];
			m_pGlobalDiffuse = m_pGlobalDiffuseView->GetDesc().pTexture;
			m_NeedsUpdate = true;
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
		TSharedRef<const TextureView>	backBuffer			= m_BackBuffers[backBufferIndex];
		const uint32 width	= backBuffer->GetDesc().pTexture->GetDesc().Width;
		const uint32 height	= backBuffer->GetDesc().pTexture->GetDesc().Height;

		commandAllocator->Reset();
		commandList->Begin(nullptr);

		if (m_AAMode == EAAMode::AAMODE_NONE)
		{
			//commandList->BlitTexture();
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
			beginRenderPassDesc.ppRenderTargets		= &backBuffer;
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

			if (m_AAMode == EAAMode::AAMODE_TAA)
			{
			}
			else if (m_AAMode == EAAMode::AAMODE_FXAA)
			{
			}

			commandList->EndRenderPass();
		}

		commandList->End();
		(*ppFirstExecutionStage) = commandList.Get();
	}
}