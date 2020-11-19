#include "Rendering/LightProbeRenderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	LightProbeRenderer::LightProbeRenderer()
		: CustomRenderer()
		, m_pRenderGraph(nullptr)
		, m_ModFrameIndex()
		, m_BackBufferCount()
		, m_ComputeCommandLists()
		, m_ComputeCommandAllocators()
		, m_SpecularFilterState(0)
		, m_SpecularFilterLayout(nullptr)
		, m_DiffuseFilterState(0)
		, m_DiffuseFilterLayout(nullptr)
	{
	}

	LightProbeRenderer::~LightProbeRenderer()
	{
	}

	bool LightProbeRenderer::Init()
	{
		{
			TSharedRef<Sampler> sampler = MakeSharedRef(Sampler::GetLinearSampler());

			DescriptorBindingDesc sourceBinding = { };
			sourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			sourceBinding.DescriptorCount	= 1;
			sourceBinding.Binding			= 0;
			sourceBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc resultBinding = { };
			resultBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			resultBinding.DescriptorCount	= 1;
			resultBinding.Binding			= 1;
			resultBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				sourceBinding,
				resultBinding,
			};

			ConstantRangeDesc pushConstantRange;
			pushConstantRange.OffsetInBytes		= 0;
			pushConstantRange.SizeInBytes		= 4;
			pushConstantRange.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			PipelineLayoutDesc specularLayoutDesc;
			specularLayoutDesc.DebugName		= "Specular Cubemap Filter Layout";
			specularLayoutDesc.ConstantRanges	=
			{
				pushConstantRange
			};
			specularLayoutDesc.DescriptorSetLayouts =
			{
				descriptorSetLayoutDesc
			};

			m_SpecularFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&specularLayoutDesc);
			if (!m_SpecularFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Specular Cubemap Filter Layout");
				DEBUGBREAK();
				return false;
			}
		}

		{
			GUID_Lambda SpecularFilterShader = ResourceManager::LoadShaderFromFile(
				"Skybox/SpecularCubemapFilter.comp", 
				FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (SpecularFilterShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedComputePipelineStateDesc specularStateDesc;
			specularStateDesc.DebugName			= "Specular Cubemap Filter State";
			specularStateDesc.PipelineLayout	= m_SpecularFilterLayout;
			specularStateDesc.Shader.ShaderGUID = SpecularFilterShader;

			m_SpecularFilterState = PipelineStateManager::CreateComputePipelineState(&specularStateDesc);
			if (m_SpecularFilterState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		{
			TSharedRef<Sampler> sampler = MakeSharedRef(Sampler::GetLinearSampler());

			DescriptorBindingDesc sourceBinding = { };
			sourceBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			sourceBinding.DescriptorCount	= 1;
			sourceBinding.Binding			= 0;
			sourceBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc resultBinding = { };
			resultBinding.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			resultBinding.DescriptorCount	= 1;
			resultBinding.Binding			= 1;
			resultBinding.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				sourceBinding,
				resultBinding,
			};

			ConstantRangeDesc pushConstantRange;
			pushConstantRange.OffsetInBytes		= 0;
			pushConstantRange.SizeInBytes		= 4;
			pushConstantRange.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			PipelineLayoutDesc diffuseLayoutDesc;
			diffuseLayoutDesc.DebugName			= "Diffuse Cubemap Filter Layout";
			diffuseLayoutDesc.ConstantRanges	=
			{
				pushConstantRange
			};
			diffuseLayoutDesc.DescriptorSetLayouts =
			{
				descriptorSetLayoutDesc
			};

			m_DiffuseFilterLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&diffuseLayoutDesc);
			if (!m_DiffuseFilterLayout)
			{
				LOG_ERROR("[LightProbeRenderer]: Failed to create Diffuse Cubemap Filter Layout");
				DEBUGBREAK();
				return false;
			}
		}

		{
			GUID_Lambda diffuseFilterShader = ResourceManager::LoadShaderFromFile(
				"Skybox/DiffuseCubemapFilter.comp",
				FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
				EShaderLang::SHADER_LANG_GLSL,
				"main");
			if (diffuseFilterShader == GUID_NONE)
			{
				DEBUGBREAK();
				return false;
			}

			ManagedComputePipelineStateDesc diffuseStateDesc;
			diffuseStateDesc.DebugName			= "Diffuse Cubemap Filter State";
			diffuseStateDesc.PipelineLayout		= m_DiffuseFilterLayout;
			diffuseStateDesc.Shader.ShaderGUID	= diffuseFilterShader;

			m_DiffuseFilterState = PipelineStateManager::CreateComputePipelineState(&diffuseStateDesc);
			if (m_DiffuseFilterState == 0)
			{
				DEBUGBREAK();
				return false;
			}
		}

		return true;
	}

	bool LightProbeRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		UNREFERENCED_VARIABLE(pPreInitDesc);
		return true;
	}

	bool LightProbeRenderer::RenderGraphPostInit()
	{
		return true;
	}

	void LightProbeRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
	}

	void LightProbeRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppPerImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void LightProbeRenderer::Render(
		uint32 modFrameIndex, 
		uint32 backBufferIndex, 
		CommandList** ppFirstExecutionStage, 
		CommandList** ppSecondaryExecutionStage, 
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppFirstExecutionStage);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(sleeping);
	}
}