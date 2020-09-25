#include "GUI/GUIPipelineStateCache.h"
#include "GUI/GUIRenderTarget.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	TArray<PipelineState*[NUM_PIPELINE_STATE_VARIATIONS]> GUIPipelineStateCache::s_PipelineStates;
	GUIRenderTarget* GUIPipelineStateCache::s_pDummyRenderTarget	= nullptr;
	PipelineLayout* GUIPipelineStateCache::s_pPipelineLayout		= nullptr;

	bool GUIPipelineStateCache::Init()
	{
		if (!InitDummyRenderTarget())
		{
			LOG_ERROR("[GUIPipelineStateCache]: Failed to initialize Dummy Render Target");
			return false;
		}

		if (!InitPipelineLayout())
		{
			LOG_ERROR("[GUIPipelineStateCache]: Failed to initialize Pipeline Layout");
			return false;
		}

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

		return success;
	}

	bool GUIPipelineStateCache::Release()
	{
		SAFEDELETE(s_pDummyRenderTarget);

		for (uint32 p = 0; p < s_PipelineStates.GetSize(); p++)
		{
			PipelineState** ppPipelineStates = s_PipelineStates[p];

			for (uint32 v = 0; v < NUM_PIPELINE_STATE_VARIATIONS; v++)
			{
				SAFERELEASE(ppPipelineStates[v]);
			}
		}

		s_PipelineStates.Clear();
		return true;
	}

	PipelineState* GUIPipelineStateCache::GetPipelineState(uint32 index, bool colorEnable, bool blendEnable)
	{
		uint32 subIndex = 0;
		subIndex |= colorEnable ? 0 : BIT(1);
		subIndex |= blendEnable ? 0 : BIT(2);

		PipelineState** ppPipelineState = &s_PipelineStates[index][subIndex];

		//Create new Pipeline State if nullptr
		if (*ppPipelineState == nullptr)
		{
			InitPipelineState(index, colorEnable, blendEnable, ppPipelineState);
		}

		return *ppPipelineState;
	}

	bool GUIPipelineStateCache::InitDummyRenderTarget()
	{
		s_pDummyRenderTarget = new GUIRenderTarget();

		GUIRenderTargetDesc renderTargetDesc = {};
		renderTargetDesc.DebugName		= "GUIPipelineStateCache Dummy RenderPass";
		renderTargetDesc.Width			= 1;
		renderTargetDesc.Height			= 1;
		renderTargetDesc.SampleCount	= 1;

		return s_pDummyRenderTarget->Init(&renderTargetDesc);
	}

	bool GUIPipelineStateCache::InitPipelineLayout()
	{
		DescriptorBindingDesc vertBufferDescriptorBindingDesc = {};
		vertBufferDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		vertBufferDescriptorBindingDesc.DescriptorCount		= 1;
		vertBufferDescriptorBindingDesc.Binding				= 0;
		vertBufferDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc vertParamsDescriptorBindingDesc = {};
		vertParamsDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		vertParamsDescriptorBindingDesc.DescriptorCount		= 1;
		vertParamsDescriptorBindingDesc.Binding				= 1;
		vertParamsDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc fragParamsDescriptorBindingDesc = {};
		fragParamsDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		fragParamsDescriptorBindingDesc.DescriptorCount		= 1;
		fragParamsDescriptorBindingDesc.Binding				= 2;
		fragParamsDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc bufferSetLayoutDesc = {};
		bufferSetLayoutDesc.DescriptorSetLayoutFlags	= FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_NONE;
		bufferSetLayoutDesc.DescriptorBindings.PushBack(vertBufferDescriptorBindingDesc);
		bufferSetLayoutDesc.DescriptorBindings.PushBack(vertParamsDescriptorBindingDesc);
		bufferSetLayoutDesc.DescriptorBindings.PushBack(fragParamsDescriptorBindingDesc);

		DescriptorBindingDesc patternDescriptorBindingDesc = {};
		patternDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		patternDescriptorBindingDesc.DescriptorCount	= 1;
		patternDescriptorBindingDesc.Binding			= 0;
		patternDescriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc rampsDescriptorBindingDesc = {};
		rampsDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		rampsDescriptorBindingDesc.DescriptorCount		= 1;
		rampsDescriptorBindingDesc.Binding				= 1;
		rampsDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc imageDescriptorBindingDesc = {};
		imageDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		imageDescriptorBindingDesc.DescriptorCount		= 1;
		imageDescriptorBindingDesc.Binding				= 2;
		imageDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc glyphsDescriptorBindingDesc = {};
		glyphsDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		glyphsDescriptorBindingDesc.DescriptorCount		= 1;
		glyphsDescriptorBindingDesc.Binding				= 3;
		glyphsDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc shadowDescriptorBindingDesc = {};
		shadowDescriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		shadowDescriptorBindingDesc.DescriptorCount		= 1;
		shadowDescriptorBindingDesc.Binding				= 4;
		shadowDescriptorBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc texturesSetLayoutDesc = {};
		texturesSetLayoutDesc.DescriptorSetLayoutFlags	= FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_NONE;
		texturesSetLayoutDesc.DescriptorBindings.PushBack(patternDescriptorBindingDesc);
		texturesSetLayoutDesc.DescriptorBindings.PushBack(rampsDescriptorBindingDesc);
		texturesSetLayoutDesc.DescriptorBindings.PushBack(imageDescriptorBindingDesc);
		texturesSetLayoutDesc.DescriptorBindings.PushBack(glyphsDescriptorBindingDesc);
		texturesSetLayoutDesc.DescriptorBindings.PushBack(shadowDescriptorBindingDesc);

		PipelineLayoutDesc pipelineLayoutDesc = {};
		pipelineLayoutDesc.DebugName			= "GUIPipelineStateCache Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { bufferSetLayoutDesc, texturesSetLayoutDesc };

		s_pPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return s_pPipelineLayout != nullptr;
	}

	bool GUIPipelineStateCache::InitPipelineState(uint32 index, bool colorEnable, bool blendEnable)
	{
		uint32 subIndex = 0;
		subIndex |= colorEnable ? 0 : BIT(1);
		subIndex |= blendEnable ? 0 : BIT(2);

		PipelineState** ppPipelineState = &s_PipelineStates[index][subIndex];

		return InitPipelineState(index, colorEnable, blendEnable, ppPipelineState);
	}

	bool GUIPipelineStateCache::InitPipelineState(uint32 index, bool colorEnable, bool blendEnable, PipelineState** ppPipelineState)
	{
		DepthStencilStateDesc depthStencilStateDesc = {};	
		depthStencilStateDesc.DepthTestEnable	= false;
		depthStencilStateDesc.DepthWriteEnable	= false;

		GraphicsPipelineStateDesc graphicsPipelineStateDesc = {};
		graphicsPipelineStateDesc.DebugName			= "GUIPipelineStateCache PipelineState"; 
		graphicsPipelineStateDesc.pRenderPass		= s_pDummyRenderTarget->GetRenderPass();
		graphicsPipelineStateDesc.pPipelineLayout	= s_pPipelineLayout;
		graphicsPipelineStateDesc.DepthStencilState	= 
		graphicsPipelineStateDesc.BlendState			
		graphicsPipelineStateDesc.RasterizerState		
		graphicsPipelineStateDesc.SampleMask			
		graphicsPipelineStateDesc.SampleCount			
		graphicsPipelineStateDesc.Subpass				

		(*ppPipelineState) = RenderAPI::GetDevice()->CreateGraphicsPipelineState();
		return true;
	}
}