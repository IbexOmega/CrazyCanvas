#include "GUI/Core/GUIPipelineStateCache.h"
#include "GUI/Core/GUIShaderManager.h"
#include "GUI/Core/GUIRenderTarget.h"
#include "GUI/Core/GUIRenderer.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	TArray<GUIPipelineStateCache::PipelineVariations> GUIPipelineStateCache::s_PipelineStates;
	RenderPass*		GUIPipelineStateCache::s_pDummyRenderPass		= nullptr;
	RenderPass*		GUIPipelineStateCache::s_pTileDummyRenderPass	= nullptr;
	PipelineLayout*	GUIPipelineStateCache::s_pPipelineLayout		= nullptr;

	bool GUIPipelineStateCache::Init(RenderPassAttachmentDesc* pBackBufferAttachmentDesc)
	{
		Release();

		if (!InitPipelineLayout())
		{
			LOG_ERROR("[GUIPipelineStateCache]: Failed to initialize Pipeline Layout");
			return false;
		}

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

		RenderPassDesc renderPassDesc = { };
		renderPassDesc.DebugName			= "GUI Dummy Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		s_pDummyRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		// TODO: Fix support for MSAA -> Or force noasis to render without -> We need to have a talk?
		colorAttachmentDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		colorAttachmentDesc.SampleCount		= FORCED_SAMPLE_COUNT;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_RENDER_TARGET;
		colorAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_RENDER_TARGET;

		RenderPassAttachmentDesc colorResolveAttachmentDesc = {};
		colorResolveAttachmentDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		colorResolveAttachmentDesc.SampleCount		= 1;
		colorResolveAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorResolveAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorResolveAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorResolveAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorResolveAttachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		colorResolveAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

		RenderPassAttachmentDesc depthStencilAttachmentDesc = {};
		depthStencilAttachmentDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthStencilAttachmentDesc.SampleCount		= FORCED_SAMPLE_COUNT;
		depthStencilAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_DONT_CARE;
		depthStencilAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_DONT_CARE;
		depthStencilAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		depthStencilAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_STORE;
		depthStencilAttachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_DONT_CARE;
		depthStencilAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		subpassDesc.RenderTargetStates =
		{
			ETextureState::TEXTURE_STATE_RENDER_TARGET,
			ETextureState::TEXTURE_STATE_DONT_CARE
		};

		subpassDesc.ResolveAttachmentStates =
		{
			ETextureState::TEXTURE_STATE_DONT_CARE,
			ETextureState::TEXTURE_STATE_RENDER_TARGET
		};

		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		renderPassDesc.DebugName			= "GUI Tile Dummy Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc, colorResolveAttachmentDesc, depthStencilAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		s_pTileDummyRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		bool success = true;

		s_PipelineStates.Resize(Noesis::Shader::Count);

		// Normal
		success = success && InitPipelineState(Noesis::Shader::RGBA, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Mask, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Path_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Path_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Path_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Path_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::SDF_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::SDF_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::SDF_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::SDF_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127V, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Pattern, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Solid, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Linear, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Radial, Noesis::StencilMode::Disabled, true, true, false);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Pattern, Noesis::StencilMode::Disabled, true, true, false);

		// Tiled
		success = success && InitPipelineState(Noesis::Shader::RGBA, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Mask, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127V, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Pattern, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Solid, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Linear, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Radial, Noesis::StencilMode::Disabled, true, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Pattern, Noesis::StencilMode::Disabled, true, true, true);

		if (!success)
		{
			LOG_ERROR("[GUIPipelineStateCache]: Failed to initialize atleast one of the Pipeline States");
		}

		return true;
	}

	bool GUIPipelineStateCache::Release()
	{
		SAFERELEASE(s_pDummyRenderPass);
		SAFERELEASE(s_pTileDummyRenderPass);
		SAFERELEASE(s_pPipelineLayout);

		for (uint32 p = 0; p < s_PipelineStates.GetSize(); p++)
		{
			PipelineVariations& pipelineVariations = s_PipelineStates[p];
			for (uint32 v = 0; v < NUM_PIPELINE_STATE_VARIATIONS; v++)
			{
				SAFERELEASE(pipelineVariations.ppVariations[v]);
			}
		}

		s_PipelineStates.Clear();
		return true;
	}

	PipelineState* GUIPipelineStateCache::GetPipelineState(uint32 index, uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled, const NoesisShaderData& shaderData)
	{
		const uint32 subIndex = CalculateSubIndex(stencilMode, colorEnable, blendEnable, tiled);
		PipelineState** ppPipelineState = &s_PipelineStates[index].ppVariations[subIndex];

		//Create new Pipeline State if nullptr
		if (*ppPipelineState == nullptr)
		{
			InitPipelineState(stencilMode, colorEnable, blendEnable, tiled, ppPipelineState, shaderData);
		}

		return *ppPipelineState;
	}

	bool GUIPipelineStateCache::InitPipelineLayout()
	{
		DescriptorBindingDesc paramsDescriptorBindingDesc = {};
		paramsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		paramsDescriptorBindingDesc.DescriptorCount			= 1;
		paramsDescriptorBindingDesc.Binding					= 0;
		paramsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc patternDescriptorBindingDesc = {};
		patternDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		patternDescriptorBindingDesc.DescriptorCount		= 1;
		patternDescriptorBindingDesc.Binding				= 1;
		patternDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc rampsDescriptorBindingDesc = {};
		rampsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		rampsDescriptorBindingDesc.DescriptorCount			= 1;
		rampsDescriptorBindingDesc.Binding					= 2;
		rampsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc imageDescriptorBindingDesc = {};
		imageDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		imageDescriptorBindingDesc.DescriptorCount			= 1;
		imageDescriptorBindingDesc.Binding					= 3;
		imageDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc glyphsDescriptorBindingDesc = {};
		glyphsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		glyphsDescriptorBindingDesc.DescriptorCount			= 1;
		glyphsDescriptorBindingDesc.Binding					= 4;
		glyphsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc shadowDescriptorBindingDesc = {};
		shadowDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		shadowDescriptorBindingDesc.DescriptorCount			= 1;
		shadowDescriptorBindingDesc.Binding					= 5;
		shadowDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayout = {};
		descriptorSetLayout.DescriptorSetLayoutFlags	= FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_NONE;
		descriptorSetLayout.DescriptorBindings.PushBack(paramsDescriptorBindingDesc);
		descriptorSetLayout.DescriptorBindings.PushBack(patternDescriptorBindingDesc);
		descriptorSetLayout.DescriptorBindings.PushBack(rampsDescriptorBindingDesc);
		descriptorSetLayout.DescriptorBindings.PushBack(imageDescriptorBindingDesc);
		descriptorSetLayout.DescriptorBindings.PushBack(glyphsDescriptorBindingDesc);
		descriptorSetLayout.DescriptorBindings.PushBack(shadowDescriptorBindingDesc);

		PipelineLayoutDesc pipelineLayoutDesc = {};
		pipelineLayoutDesc.DebugName			= "GUIPipelineStateCache Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayout };

		s_pPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return s_pPipelineLayout != nullptr;
	}

	bool GUIPipelineStateCache::InitPipelineState(uint32 index, uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled)
	{
		const uint32 subIndex = CalculateSubIndex(stencilMode, colorEnable, blendEnable, tiled);
		PipelineState** ppPipelineState = &s_PipelineStates[index].ppVariations[subIndex];
		NoesisShaderData shaderData = NoesisGetShaderData(index);

		return InitPipelineState(stencilMode, colorEnable, blendEnable, tiled, ppPipelineState, shaderData);
	}

	bool GUIPipelineStateCache::InitPipelineState(uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled, PipelineState** ppPipelineState, const NoesisShaderData& shaderData)
	{
		StencilOpStateDesc stencilOpStateDesc = {};
		stencilOpStateDesc.FailOp			= EStencilOp::STENCIL_OP_KEEP;
		stencilOpStateDesc.DepthFailOp		= EStencilOp::STENCIL_OP_KEEP;
		stencilOpStateDesc.CompareOp		= ECompareOp::COMPARE_OP_EQUAL;
		stencilOpStateDesc.CompareMask		= 0xFFFFFFFF;
		stencilOpStateDesc.WriteMask		= 0xFFFFFFFF;
		stencilOpStateDesc.Reference		= 0x00000000;

		bool enableStencil = NoesisStencilOpToLambdaStencilOp(stencilMode, stencilOpStateDesc.PassOp);

		DepthStencilStateDesc depthStencilStateDesc = {};	
		depthStencilStateDesc.DepthTestEnable	= false;
		depthStencilStateDesc.DepthWriteEnable	= false;
		depthStencilStateDesc.FrontFace			= stencilOpStateDesc;
		depthStencilStateDesc.BackFace			= stencilOpStateDesc;
		depthStencilStateDesc.StencilTestEnable = enableStencil;
		
		BlendAttachmentStateDesc blendAttachmentStateDesc = {};
		blendAttachmentStateDesc.BlendOp		= EBlendOp::BLEND_OP_ADD,
		blendAttachmentStateDesc.SrcBlend		= EBlendFactor::BLEND_FACTOR_ONE,
		blendAttachmentStateDesc.DstBlend		= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
		blendAttachmentStateDesc.BlendOpAlpha	= EBlendOp::BLEND_OP_ADD,
		blendAttachmentStateDesc.SrcBlendAlpha	= EBlendFactor::BLEND_FACTOR_ONE,
		blendAttachmentStateDesc.DstBlendAlpha	= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
		blendAttachmentStateDesc.BlendEnabled	= blendEnable;
		blendAttachmentStateDesc.RenderTargetComponentMask	= colorEnable ? COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A : COLOR_COMPONENT_FLAG_NONE;

		BlendStateDesc blendStateDesc = {};
		
		// Could not use a ternary. Missing C++ feature?
		if (tiled)
			blendStateDesc.BlendAttachmentStates = { blendAttachmentStateDesc, blendAttachmentStateDesc };
		else
			blendStateDesc.BlendAttachmentStates = { blendAttachmentStateDesc };

		blendStateDesc.BlendConstants[0]		= 1.0f;
		blendStateDesc.BlendConstants[1]		= 1.0f;
		blendStateDesc.BlendConstants[2]		= 1.0f;
		blendStateDesc.BlendConstants[3]		= 1.0f;
		blendStateDesc.LogicOp					= ELogicOp::LOGIC_OP_COPY;
		blendStateDesc.AlphaToCoverageEnable	= false;
		blendStateDesc.AlphaToOneEnable			= false;
		blendStateDesc.LogicOpEnable			= false;

		GraphicsPipelineStateDesc graphicsPipelineStateDesc = {};
		graphicsPipelineStateDesc.DebugName					= "GUIPipelineStateCache PipelineState"; 
		graphicsPipelineStateDesc.pRenderPass				= tiled ? s_pTileDummyRenderPass : s_pDummyRenderPass;
		graphicsPipelineStateDesc.pPipelineLayout			= s_pPipelineLayout;
		graphicsPipelineStateDesc.DepthStencilState			= depthStencilStateDesc;
		graphicsPipelineStateDesc.BlendState				= blendStateDesc;
		graphicsPipelineStateDesc.SampleMask				= 0xFFFFFFFF;
		graphicsPipelineStateDesc.SampleCount				= 1;
		graphicsPipelineStateDesc.Subpass					= 0;
		graphicsPipelineStateDesc.ExtraDynamicState			= EXTRA_DYNAMIC_STATE_FLAG_STENCIL_REFERENCE;// | EXTRA_DYNAMIC_STATE_FLAG_STENCIL_ENABLE | EXTRA_DYNAMIC_STATE_FLAG_STENCIL_OP;
		graphicsPipelineStateDesc.VertexShader.pShader		= ResourceManager::GetShader(GUIShaderManager::GetGUIVertexShaderGUID(shaderData.VertexShaderID));
		graphicsPipelineStateDesc.PixelShader.pShader		= ResourceManager::GetShader(GUIShaderManager::GetGUIPixelShaderGUID(shaderData.PixelShaderID));
		graphicsPipelineStateDesc.RasterizerState.CullMode	= ECullMode::CULL_MODE_NONE;

#ifdef FORCED_SAMPLE_COUNT
		if (tiled && FORCED_SAMPLE_COUNT > 1)
		{
			graphicsPipelineStateDesc.RasterizerState.MultisampleEnable	= true;
			graphicsPipelineStateDesc.SampleCount						= FORCED_SAMPLE_COUNT;
		}
#endif


		uint32 offset = 0;

		//Position
		{
			InputElementDesc posElementDesc = {};
			posElementDesc.Semantic		= "POSITION";
			posElementDesc.Binding		= 0;
			posElementDesc.Stride		= shaderData.VertexSize;
			posElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			posElementDesc.Location		= 0;
			posElementDesc.Offset		= 0;
			posElementDesc.Format		= EFormat::FORMAT_R32G32_SFLOAT;
			graphicsPipelineStateDesc.InputLayout.PushBack(posElementDesc);
			offset += TextureFormatStride(posElementDesc.Format);
		}

		if (shaderData.VertexFormat & NOESIS_VF_COLOR)
		{
			InputElementDesc colorElementDesc = {};
			colorElementDesc.Semantic	= "COLOR";
			colorElementDesc.Binding	= 0;
			colorElementDesc.Stride		= shaderData.VertexSize;
			colorElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			colorElementDesc.Location	= 1;
			colorElementDesc.Offset		= offset;
			colorElementDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
			graphicsPipelineStateDesc.InputLayout.PushBack(colorElementDesc);
			offset += TextureFormatStride(colorElementDesc.Format);
		}

		if (shaderData.VertexFormat & NOESIS_VF_TEX0)
		{
			InputElementDesc tex0ElementDesc = {};
			tex0ElementDesc.Semantic	= "TEX_0";
			tex0ElementDesc.Binding		= 0;
			tex0ElementDesc.Stride		= shaderData.VertexSize;
			tex0ElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			tex0ElementDesc.Location	= 2;
			tex0ElementDesc.Offset		= offset;
			tex0ElementDesc.Format		= EFormat::FORMAT_R32G32_SFLOAT;
			graphicsPipelineStateDesc.InputLayout.PushBack(tex0ElementDesc);
			offset += TextureFormatStride(tex0ElementDesc.Format);
		}

		if (shaderData.VertexFormat & NOESIS_VF_TEX1)
		{
			InputElementDesc tex1ElementDesc = {};
			tex1ElementDesc.Semantic	= "TEX_1";
			tex1ElementDesc.Binding		= 0;
			tex1ElementDesc.Stride		= shaderData.VertexSize;
			tex1ElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			tex1ElementDesc.Location	= 3;
			tex1ElementDesc.Offset		= offset;
			tex1ElementDesc.Format		= EFormat::FORMAT_R32G32_SFLOAT;
			graphicsPipelineStateDesc.InputLayout.PushBack(tex1ElementDesc);
			offset += TextureFormatStride(tex1ElementDesc.Format);
		}

		if (shaderData.VertexFormat & NOESIS_VF_TEX2)
		{
			InputElementDesc tex2ElementDesc = {};
			tex2ElementDesc.Semantic	= "TEX_2";
			tex2ElementDesc.Binding		= 0;
			tex2ElementDesc.Stride		= shaderData.VertexSize;
			tex2ElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			tex2ElementDesc.Location	= 4;
			tex2ElementDesc.Offset		= offset;
			tex2ElementDesc.Format		= EFormat::FORMAT_R16G16B16A16_SFLOAT;
			graphicsPipelineStateDesc.InputLayout.PushBack(tex2ElementDesc);
			offset += TextureFormatStride(tex2ElementDesc.Format);
		}

		if (shaderData.VertexFormat & NOESIS_VF_COVERAGE)
		{
			InputElementDesc coverageElementDesc = {};
			coverageElementDesc.Semantic	= "COVERAGE";
			coverageElementDesc.Binding		= 0;
			coverageElementDesc.Stride		= shaderData.VertexSize;
			coverageElementDesc.InputRate	= EVertexInputRate::VERTEX_INPUT_PER_VERTEX;
			coverageElementDesc.Location	= 5;
			coverageElementDesc.Offset		= offset;
			coverageElementDesc.Format		= EFormat::FORMAT_R32_SFLOAT;
			graphicsPipelineStateDesc.InputLayout.PushBack(coverageElementDesc);
			offset += TextureFormatStride(coverageElementDesc.Format);
		}

		(*ppPipelineState) = RenderAPI::GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc);
		return true;
	}

	uint32 GUIPipelineStateCache::CalculateSubIndex(uint8 stencilMode, bool colorEnable, bool blendEnable, bool tiled)
	{
		uint32 subIndex = 0;
		if (stencilMode == Noesis::StencilMode::Equal_Keep)
			subIndex += 8;
		else if (stencilMode == Noesis::StencilMode::Equal_Incr)
			subIndex += 16;
		else if (stencilMode == Noesis::StencilMode::Equal_Decr)
			subIndex += 24;

		if (!colorEnable)	
			subIndex += 1;
		if (!blendEnable)	
			subIndex += 2;
		if (tiled)
			subIndex += 4;
		return subIndex;
	}
}