#include "Log/Log.h"

#include "Containers/TArray.h"

#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

namespace LambdaEngine
{
	GraphicsPipelineStateVK::GraphicsPipelineStateVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	GraphicsPipelineStateVK::~GraphicsPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool GraphicsPipelineStateVK::Init(const GraphicsPipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		const PipelineLayoutVK* pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
		const RenderPassVK*		pRenderPassVk		= reinterpret_cast<const RenderPassVK*>(pDesc->pRenderPass);

		 // Define shader stage create infos
		TArray<VkPipelineShaderStageCreateInfo>		shaderStagesInfos;
		TArray<VkSpecializationInfo>				shaderStagesSpecializationInfos;
		TArray<TArray<VkSpecializationMapEntry>>	shaderStagesSpecializationMaps;

		// Mesh-Shader Pipeline
		if (pDesc->MeshShader.pShader != nullptr)
		{
			CreateShaderStageInfo(&pDesc->MeshShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(&pDesc->TaskShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
		}
		else if (pDesc->VertexShader.pShader != nullptr)
		{
			// Vertex-Shader Pipeline
			CreateShaderStageInfo(&pDesc->VertexShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(&pDesc->HullShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(&pDesc->DomainShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(&pDesc->GeometryShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
		}

		// Pixel-Shader
		CreateShaderStageInfo(&pDesc->PixelShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

		// Default InputAssembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { };
		inputAssembly.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.pNext						= nullptr;
		inputAssembly.flags						= 0;
		inputAssembly.topology					= ConvertPrimitiveToplogy(pDesc->InputAssembly.PrimitiveTopology);
		inputAssembly.primitiveRestartEnable	= (pDesc->InputAssembly.PrimitiveRestartEnable) ? VK_TRUE : VK_FALSE;

		// Default RasterizerState
		VkPipelineRasterizationStateCreateInfo rasterizerState = { };
		rasterizerState.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerState.flags					= 0;
		rasterizerState.pNext					= nullptr;
		rasterizerState.polygonMode				= ConvertPolygonMode(pDesc->RasterizerState.PolygonMode);
		rasterizerState.lineWidth				= pDesc->RasterizerState.LineWidth;
		rasterizerState.cullMode				= ConvertCullMode(pDesc->RasterizerState.CullMode);
		rasterizerState.frontFace				= (pDesc->RasterizerState.FrontFaceCounterClockWise) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		rasterizerState.depthBiasEnable			= (pDesc->RasterizerState.DepthBiasEnable)			? VK_TRUE : VK_FALSE;
		rasterizerState.depthClampEnable		= (pDesc->RasterizerState.DepthClampEnable)			? VK_TRUE : VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = (pDesc->RasterizerState.RasterizerDiscardEnable)	? VK_TRUE : VK_FALSE;
		rasterizerState.depthBiasClamp			= pDesc->RasterizerState.DepthBiasClamp;
		rasterizerState.depthBiasConstantFactor = pDesc->RasterizerState.DepthBiasConstantFactor;
		rasterizerState.depthBiasSlopeFactor	= pDesc->RasterizerState.DepthBiasSlopeFactor;

		// MultisamplingState
		VkPipelineMultisampleStateCreateInfo multisamplingState = { };
		multisamplingState.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingState.flags					= 0;
		multisamplingState.pNext					= nullptr;
		multisamplingState.alphaToCoverageEnable	= (pDesc->BlendState.AlphaToCoverageEnable) ? VK_TRUE : VK_FALSE;
		multisamplingState.alphaToOneEnable			= (pDesc->BlendState.AlphaToOneEnable)		? VK_TRUE : VK_FALSE;
		multisamplingState.minSampleShading			= 0.0f;
		multisamplingState.sampleShadingEnable		= (pDesc->RasterizerState.MultisampleEnable) ? VK_TRUE : VK_FALSE;
		multisamplingState.rasterizationSamples		= ConvertSampleCount(pDesc->SampleCount);

		VkSampleMask sampleMask			= pDesc->SampleMask;
		multisamplingState.pSampleMask	= &sampleMask;

		// BlendState
		const uint32 blendAttachmentCount = static_cast<uint32>(pDesc->BlendState.BlendAttachmentStates.GetSize());
		TArray<VkPipelineColorBlendAttachmentState> blendAttachments(blendAttachmentCount);
		for (uint32 i = 0; i < blendAttachmentCount; i++)
		{
			VkPipelineColorBlendAttachmentState*	pAttachmentVk	= &blendAttachments[i];
			const BlendAttachmentStateDesc*			pAttachment		= &pDesc->BlendState.BlendAttachmentStates[i];

			pAttachmentVk->blendEnable			= (pAttachment->BlendEnabled) ? VK_TRUE : VK_FALSE;
			pAttachmentVk->colorWriteMask		= ConvertColorComponentMask(pAttachment->RenderTargetComponentMask);
			pAttachmentVk->colorBlendOp			= ConvertBlendOp(pAttachment->BlendOp);
			pAttachmentVk->srcColorBlendFactor	= ConvertBlendFactor(pAttachment->SrcBlend);
			pAttachmentVk->dstColorBlendFactor	= ConvertBlendFactor(pAttachment->DstBlend);
			pAttachmentVk->alphaBlendOp			= ConvertBlendOp(pAttachment->BlendOpAlpha);
			pAttachmentVk->srcAlphaBlendFactor	= ConvertBlendFactor(pAttachment->SrcBlendAlpha);
			pAttachmentVk->dstAlphaBlendFactor	= ConvertBlendFactor(pAttachment->DstBlendAlpha);
		}

		VkPipelineColorBlendStateCreateInfo blendState = { };
		blendState.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendState.flags				= 0;
		blendState.pNext				= nullptr;
		blendState.logicOp				= ConvertLogicOp(pDesc->BlendState.LogicOp);
		blendState.logicOpEnable		= (pDesc->BlendState.LogicOpEnable) ? VK_TRUE : VK_FALSE;
		blendState.pAttachments			= blendAttachments.GetData();
		blendState.attachmentCount		= blendAttachmentCount;
		memcpy(blendState.blendConstants, pDesc->BlendState.BlendConstants, sizeof(float) * 4);

		// DepthstencilState
		VkPipelineDepthStencilStateCreateInfo depthStencilState = { };
		depthStencilState.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.flags					= 0;
		depthStencilState.pNext					= nullptr;
		depthStencilState.depthTestEnable		= pDesc->DepthStencilState.DepthTestEnable ? VK_TRUE : VK_FALSE;
		depthStencilState.depthWriteEnable		= pDesc->DepthStencilState.DepthWriteEnable ? VK_TRUE : VK_FALSE;
		depthStencilState.depthCompareOp		= ConvertCompareOp(pDesc->DepthStencilState.CompareOp);
		depthStencilState.depthBoundsTestEnable = pDesc->DepthStencilState.DepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
		depthStencilState.stencilTestEnable		= pDesc->DepthStencilState.StencilTestEnable ? VK_TRUE : VK_FALSE;;
		depthStencilState.minDepthBounds		= pDesc->DepthStencilState.MinDepthBounds;
		depthStencilState.maxDepthBounds		= pDesc->DepthStencilState.MaxDepthBounds;
		depthStencilState.front					= ConvertStencilOpState(pDesc->DepthStencilState.FrontFace);
		depthStencilState.back					= ConvertStencilOpState(pDesc->DepthStencilState.BackFace);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.flags = 0;
		vertexInputInfo.pNext = nullptr;

		TArray<VkVertexInputBindingDescription>		bindingDescriptors;
		TArray<VkVertexInputAttributeDescription>	attributeDescriptors;
		bindingDescriptors.Reserve(pDesc->InputLayout.GetSize());

		if (!pDesc->InputLayout.IsEmpty())
		{
			for (const InputElementDesc& inputElement : pDesc->InputLayout)
			{
				VkVertexInputAttributeDescription vkInputAttributeDesc = {};
				vkInputAttributeDesc.location	= inputElement.Location;
				vkInputAttributeDesc.binding	= inputElement.Binding;
				vkInputAttributeDesc.format		= ConvertFormat(inputElement.Format);
				vkInputAttributeDesc.offset		= inputElement.Offset;
				attributeDescriptors.EmplaceBack(vkInputAttributeDesc);

				VkVertexInputBindingDescription vkInputBindingDesc = {};
				vkInputBindingDesc.binding	 = inputElement.Binding;
				vkInputBindingDesc.stride	 = inputElement.Stride;
				vkInputBindingDesc.inputRate = ConvertVertexInputRate(inputElement.InputRate);
				bindingDescriptors.EmplaceBack(vkInputBindingDesc);
			}

			vertexInputInfo.vertexAttributeDescriptionCount	= static_cast<uint32>(attributeDescriptors.GetSize());
			vertexInputInfo.pVertexAttributeDescriptions	= attributeDescriptors.GetData();
			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32>(bindingDescriptors.GetSize());
			vertexInputInfo.pVertexBindingDescriptions		= bindingDescriptors.GetData();
		}
		else
		{
			vertexInputInfo.vertexAttributeDescriptionCount	= 0;
			vertexInputInfo.pVertexAttributeDescriptions	= nullptr;
			vertexInputInfo.vertexBindingDescriptionCount	= 0;
			vertexInputInfo.pVertexBindingDescriptions		= nullptr;
		}

		// Viewport count
		// TODO: If we need more than one viewport, we need to support this somehow
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.flags			= 0;
		viewportState.pNext			= nullptr;
		viewportState.viewportCount	= 1;
		viewportState.pViewports	= nullptr;
		viewportState.scissorCount	= 1;
		viewportState.pScissors		= nullptr;

		// Dynamic state
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.flags				= 0;
		dynamicState.pNext				= nullptr;
		dynamicState.pDynamicStates		= dynamicStates;
		dynamicState.dynamicStateCount	= 2;

		// Pipeline info
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext					= nullptr;
		pipelineInfo.flags					= 0;
		pipelineInfo.stageCount				= uint32_t(shaderStagesInfos.GetSize());
		pipelineInfo.pStages				= shaderStagesInfos.GetData();
		pipelineInfo.pVertexInputState		= &vertexInputInfo;
		pipelineInfo.pInputAssemblyState	= &inputAssembly;
		pipelineInfo.pViewportState			= &viewportState;
		pipelineInfo.pRasterizationState	= &rasterizerState;
		pipelineInfo.pMultisampleState		= &multisamplingState;
		pipelineInfo.pDepthStencilState		= &depthStencilState;
		pipelineInfo.pColorBlendState		= &blendState;
		pipelineInfo.pDynamicState			= &dynamicState;
		pipelineInfo.renderPass				= pRenderPassVk->GetRenderPass();
		pipelineInfo.layout					= pPipelineLayoutVk->GetPipelineLayout();
		pipelineInfo.subpass				= 0;
		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex		= -1;

		VkResult result = vkCreateGraphicsPipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed for %s", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed");
			}
			
			return false;
		}
		else
		{
			SetName(pDesc->DebugName);
			
			if (!pDesc->DebugName.empty())
			{
				D_LOG_MESSAGE("[GraphicsPipelineStateVK]: Created Pipeline for %s", pDesc->DebugName.c_str());
			}
			else
			{
				D_LOG_MESSAGE("[GraphicsPipelineStateVK]: Created Pipeline");
			}
			
			return true;
		}
	}

	void GraphicsPipelineStateVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_Pipeline), VK_OBJECT_TYPE_PIPELINE);
		m_DebugName = debugName;
	}
	
	void GraphicsPipelineStateVK::CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
		TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pShaderModule->pShader);
		if (!pShader)
		{
			return;
		}

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= ConvertShaderStageFlag(pShader->GetDesc().Stage);
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint().c_str();

		// Shader Constants
		if (!pShaderModule->ShaderConstants.IsEmpty())
		{
			TArray<VkSpecializationMapEntry> specializationEntires(pShaderModule->ShaderConstants.GetSize());
			for (uint32 i = 0; i < pShaderModule->ShaderConstants.GetSize(); i++)
			{
				VkSpecializationMapEntry specializationEntry = { };
				specializationEntry.constantID	= i;
				specializationEntry.offset		= i * sizeof(ShaderConstant);
				specializationEntry.size		= sizeof(ShaderConstant);
				specializationEntires.EmplaceBack(specializationEntry);
			}

			shaderStagesSpecializationMaps.EmplaceBack(specializationEntires);

			VkSpecializationInfo specializationInfo = { };
			specializationInfo.mapEntryCount	= static_cast<uint32>(specializationEntires.GetSize());
			specializationInfo.pMapEntries		= specializationEntires.GetData();
			specializationInfo.dataSize			= static_cast<uint32>(pShaderModule->ShaderConstants.GetSize()) * sizeof(ShaderConstant);
			specializationInfo.pData			= pShaderModule->ShaderConstants.GetData();
			shaderStagesSpecializationInfos.EmplaceBack(specializationInfo);

			shaderCreateInfo.pSpecializationInfo = &shaderStagesSpecializationInfos.GetBack();
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		shaderStagesInfos.EmplaceBack(shaderCreateInfo);
	}
}
