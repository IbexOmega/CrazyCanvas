#include "GUI/GUIPipelineStateCache.h"
#include "GUI/GUIShaderManager.h"
#include "GUI/GUIRenderTarget.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineState.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	constexpr const uint32 SAMPLE_COUNT_1_INDEX = 0;
	constexpr const uint32 SAMPLE_COUNT_2_INDEX = 1;
	constexpr const uint32 SAMPLE_COUNT_4_INDEX = 2;

	TArray<GUIPipelineStateCache::PipelineVariations> GUIPipelineStateCache::s_PipelineStates;
	RenderPass* GUIPipelineStateCache::s_pDummyRenderPass		= nullptr;
	PipelineLayout* GUIPipelineStateCache::s_pPipelineLayout	= nullptr;

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

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "GUI Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		s_pDummyRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		bool success = true;

		s_PipelineStates.Resize(Noesis::Shader::Count);
		success = success && InitPipelineState(Noesis::Shader::RGBA, true, true);
		success = success && InitPipelineState(Noesis::Shader::Mask, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Path_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::PathAA_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::SDF_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Opacity_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow35H_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow63H_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Shadow127H_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127V, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur35H_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur63H_Pattern, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Solid, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Linear, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Radial, true, true);
		success = success && InitPipelineState(Noesis::Shader::Image_Blur127H_Pattern, true, true);

		if (!success)
		{
			LOG_ERROR("[GUIPipelineStateCache]: Failed to initialize atleast one of the Pipeline States");
		}

		return false;
	}

	bool GUIPipelineStateCache::Release()
	{
		SAFERELEASE(s_pDummyRenderPass);

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

	PipelineState* GUIPipelineStateCache::GetPipelineState(uint32 index, bool colorEnable, bool blendEnable, const NoesisShaderData& shaderData)
	{
		uint32 subIndex = CalculateSubIndex(colorEnable, blendEnable);
		PipelineState** ppPipelineState = &s_PipelineStates[index].ppVariations[subIndex];

		//Create new Pipeline State if nullptr
		if (*ppPipelineState == nullptr)
		{
			InitPipelineState(index, colorEnable, blendEnable, ppPipelineState, shaderData);
		}

		return *ppPipelineState;
	}

	bool GUIPipelineStateCache::InitPipelineLayout()
	{
		DescriptorBindingDesc vertBufferDescriptorBindingDesc = {};
		vertBufferDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		vertBufferDescriptorBindingDesc.DescriptorCount		= 1;
		vertBufferDescriptorBindingDesc.Binding				= 0;
		vertBufferDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc paramsDescriptorBindingDesc = {};
		paramsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		paramsDescriptorBindingDesc.DescriptorCount			= 1;
		paramsDescriptorBindingDesc.Binding					= 1;
		paramsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc patternDescriptorBindingDesc = {};
		patternDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		patternDescriptorBindingDesc.DescriptorCount		= 1;
		patternDescriptorBindingDesc.Binding				= 2;
		patternDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc rampsDescriptorBindingDesc = {};
		rampsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		rampsDescriptorBindingDesc.DescriptorCount			= 1;
		rampsDescriptorBindingDesc.Binding					= 3;
		rampsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc imageDescriptorBindingDesc = {};
		imageDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		imageDescriptorBindingDesc.DescriptorCount			= 1;
		imageDescriptorBindingDesc.Binding					= 4;
		imageDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc glyphsDescriptorBindingDesc = {};
		glyphsDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		glyphsDescriptorBindingDesc.DescriptorCount			= 1;
		glyphsDescriptorBindingDesc.Binding					= 5;
		glyphsDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc shadowDescriptorBindingDesc = {};
		shadowDescriptorBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		shadowDescriptorBindingDesc.DescriptorCount			= 1;
		shadowDescriptorBindingDesc.Binding					= 6;
		shadowDescriptorBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayout = {};
		descriptorSetLayout.DescriptorSetLayoutFlags	= FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_NONE;
		descriptorSetLayout.DescriptorBindings.PushBack(vertBufferDescriptorBindingDesc);
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

	bool GUIPipelineStateCache::InitPipelineState(uint32 index, bool colorEnable, bool blendEnable)
	{
		uint32 subIndex = CalculateSubIndex(colorEnable, blendEnable);
		PipelineState** ppPipelineState = &s_PipelineStates[index].ppVariations[subIndex];
		NoesisShaderData shaderData = NoesisGetShaderData(index);

		return InitPipelineState(index, colorEnable, blendEnable, ppPipelineState, shaderData);
	}

	bool GUIPipelineStateCache::InitPipelineState(uint32 index, bool colorEnable, bool blendEnable, PipelineState** ppPipelineState, const NoesisShaderData& shaderData)
	{
		StencilOpStateDesc stencilOpStateDesc = {};
		stencilOpStateDesc.FailOp			= EStencilOp::STENCIL_OP_KEEP;
		stencilOpStateDesc.PassOp			= EStencilOp::STENCIL_OP_KEEP;
		stencilOpStateDesc.DepthFailOp		= EStencilOp::STENCIL_OP_KEEP;
		stencilOpStateDesc.CompareOp		= ECompareOp::COMPARE_OP_EQUAL;
		stencilOpStateDesc.CompareMask		= 0xFFFFFFFF;
		stencilOpStateDesc.WriteMask		= 0xFFFFFFFF;
		stencilOpStateDesc.Reference		= 0x00000000;

		DepthStencilStateDesc depthStencilStateDesc = {};	
		depthStencilStateDesc.DepthTestEnable	= false;
		depthStencilStateDesc.DepthWriteEnable	= false;
		depthStencilStateDesc.FrontFace			= stencilOpStateDesc;
		depthStencilStateDesc.BackFace			= stencilOpStateDesc;
		
		BlendAttachmentStateDesc blendAttachmentStateDesc = {};
		blendAttachmentStateDesc.BlendEnabled				= blendEnable;
		blendAttachmentStateDesc.RenderTargetComponentMask	= colorEnable ? COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A : COLOR_COMPONENT_FLAG_NONE;

		BlendStateDesc blendStateDesc = {};
		blendStateDesc.BlendAttachmentStates	= { blendAttachmentStateDesc};
		blendStateDesc.BlendConstants[0]		= 1.0f;
		blendStateDesc.BlendConstants[1]		= 1.0f;
		blendStateDesc.BlendConstants[2]		= 1.0f;
		blendStateDesc.BlendConstants[3]		= 1.0f;
		blendStateDesc.LogicOp					= ELogicOp::LOGIC_OP_COPY;
		blendStateDesc.AlphaToCoverageEnable	= false;
		blendStateDesc.AlphaToOneEnable			= false;
		blendStateDesc.LogicOpEnable			= false;

		GraphicsPipelineStateDesc graphicsPipelineStateDesc = {};
		graphicsPipelineStateDesc.DebugName				= "GUIPipelineStateCache PipelineState"; 
		graphicsPipelineStateDesc.pRenderPass			= s_pDummyRenderPass;
		graphicsPipelineStateDesc.pPipelineLayout		= s_pPipelineLayout;
		graphicsPipelineStateDesc.DepthStencilState		= depthStencilStateDesc;
		graphicsPipelineStateDesc.BlendState			= blendStateDesc;
		graphicsPipelineStateDesc.SampleMask			= 0xFFFFFFFF;
		graphicsPipelineStateDesc.SampleCount			= 1;
		graphicsPipelineStateDesc.Subpass				= 0;
		graphicsPipelineStateDesc.ExtraDynamicState		= EXTRA_DYNAMIC_STATE_FLAG_STENCIL_ENABLE | EXTRA_DYNAMIC_STATE_FLAG_STENCIL_OP | EXTRA_DYNAMIC_STATE_FLAG_STENCIL_REFERENCE;
		graphicsPipelineStateDesc.VertexShader.pShader	= ResourceManager::GetShader(GUIShaderManager::GetGUIVertexShaderGUID(shaderData.VertexShaderID));
		graphicsPipelineStateDesc.PixelShader.pShader	= ResourceManager::GetShader(GUIShaderManager::GetGUIPixelShaderGUID(shaderData.PixelShaderID));

		(*ppPipelineState) = RenderAPI::GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc);
		return true;
	}

	uint32 GUIPipelineStateCache::CalculateSubIndex(bool colorEnable, bool blendEnable)
	{
		uint32 subIndex = 0;
		if (!colorEnable)			subIndex += 1;
		if (!blendEnable)			subIndex += 2;
		return subIndex;
	}
}