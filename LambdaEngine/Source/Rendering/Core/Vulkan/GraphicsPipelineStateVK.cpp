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
		if (pDesc->pMeshShader != nullptr)
		{
			CreateShaderStageInfo(pDesc->pMeshShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(pDesc->pTaskShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
		}
		else if (pDesc->pVertexShader)
		{
			// Vertex-Shader Pipeline
			CreateShaderStageInfo(pDesc->pVertexShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(pDesc->pHullShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(pDesc->pDomainShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
			CreateShaderStageInfo(pDesc->pGeometryShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);
		}

		// Pixel-Shader
		CreateShaderStageInfo(pDesc->pPixelShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

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
		VkPipelineColorBlendAttachmentState* pBlendAttachments = DBG_NEW VkPipelineColorBlendAttachmentState[pDesc->BlendState.BlendAttachmentStateCount];
		for (uint32 i = 0; i < pDesc->BlendState.BlendAttachmentStateCount; i++)
		{
			VkPipelineColorBlendAttachmentState*	pAttachmentVk	= pBlendAttachments + i;
			const BlendAttachmentStateDesc*			pAttachment		= pDesc->BlendState.pBlendAttachmentStates + i;

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
		blendState.pAttachments			= pBlendAttachments;
		blendState.attachmentCount		= pDesc->BlendState.BlendAttachmentStateCount;
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
		bindingDescriptors.reserve(pDesc->VertexInputBindingCount);

		if (pDesc->VertexInputBindingCount > 0)
		{
			for (uint32 b = 0; b < pDesc->VertexInputBindingCount; b++)
			{
				const VertexInputBindingDesc* pInputBindingDesc = &pDesc->pVertexInputBindings[b];

				VkVertexInputBindingDescription vkInputBindingDesc = {};
				vkInputBindingDesc.binding	 = pInputBindingDesc->Binding;
				vkInputBindingDesc.stride	 = pInputBindingDesc->Stride;
				vkInputBindingDesc.inputRate = ConvertVertexInputRate(pInputBindingDesc->InputRate);
				bindingDescriptors.push_back(vkInputBindingDesc);

				for (uint32 a = 0; a < pInputBindingDesc->AttributeCount; a++)
				{
					const VertexInputAttributeDesc* pInputAttributeDesc = &pInputBindingDesc->pAttributes[a];

					VkVertexInputAttributeDescription vkInputAttributeDesc = {};
					vkInputAttributeDesc.location	= pInputAttributeDesc->Location;
					vkInputAttributeDesc.binding	= pInputBindingDesc->Binding;
					vkInputAttributeDesc.format		= ConvertFormat(pInputAttributeDesc->Format);
					vkInputAttributeDesc.offset		= pInputAttributeDesc->Offset;
					attributeDescriptors.push_back(vkInputAttributeDesc);
				}
			}

			vertexInputInfo.vertexAttributeDescriptionCount	= (uint32)attributeDescriptors.size();
			vertexInputInfo.pVertexAttributeDescriptions	= attributeDescriptors.data();
			vertexInputInfo.vertexBindingDescriptionCount	= (uint32)bindingDescriptors.size();
			vertexInputInfo.pVertexBindingDescriptions		= bindingDescriptors.data();
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
		pipelineInfo.stageCount				= uint32_t(shaderStagesInfos.size());
		pipelineInfo.pStages				= shaderStagesInfos.data();
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
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed for %s", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed");
			}
			
			return false;
		}
		else
		{
			SetName(pDesc->pName);
			
			if (pDesc->pName)
			{
				D_LOG_MESSAGE("[GraphicsPipelineStateVK]: Created Pipeline for %s", pDesc->pName);
			}
			else
			{
				D_LOG_MESSAGE("[GraphicsPipelineStateVK]: Created Pipeline");
			}
			
			return true;
		}
	}

	void GraphicsPipelineStateVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Pipeline, VK_OBJECT_TYPE_PIPELINE);
		}
	}
	
	void GraphicsPipelineStateVK::CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
		TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pShaderModule->pShader);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= ConvertShaderStageFlag(pShader->GetDesc().Stage);
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint();

		// Shader Constants
		if (pShaderModule->ShaderConstantCount)
		{
			TArray<VkSpecializationMapEntry> specializationEntires(pShaderModule->ShaderConstantCount);
			for (uint32 i = 0; i < pShaderModule->ShaderConstantCount; i++)
			{
				VkSpecializationMapEntry specializationEntry = {};
				specializationEntry.constantID	= i;
				specializationEntry.offset		= i * sizeof(ShaderConstant);
				specializationEntry.size		= sizeof(ShaderConstant);
				specializationEntires.emplace_back(specializationEntry);
			}

			shaderStagesSpecializationMaps.emplace_back(specializationEntires);

			VkSpecializationInfo specializationInfo = { };
			specializationInfo.mapEntryCount	= static_cast<uint32>(specializationEntires.size());
			specializationInfo.pMapEntries		= specializationEntires.data();
			specializationInfo.dataSize			= pShaderModule->ShaderConstantCount * sizeof(ShaderConstant);
			specializationInfo.pData			= pShaderModule->pConstants;
			shaderStagesSpecializationInfos.emplace_back(specializationInfo);

			shaderCreateInfo.pSpecializationInfo = &shaderStagesSpecializationInfos.back();
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		shaderStagesInfos.emplace_back(shaderCreateInfo);
	}
}
