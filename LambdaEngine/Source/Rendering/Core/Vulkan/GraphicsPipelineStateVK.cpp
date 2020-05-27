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
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, pDesc))
		{
			return false;
		}

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

		VkSampleMask sampleMask = pDesc->SampleMask;
		multisamplingState.pSampleMask = &sampleMask;

		// BlendState
		VkPipelineColorBlendAttachmentState* pBlendAttachments = DBG_NEW VkPipelineColorBlendAttachmentState[pDesc->BlendState.BlendAttachmentStateCount];
		for (uint32 i = 0; i < pDesc->BlendState.BlendAttachmentStateCount; i++)
		{
			VkPipelineColorBlendAttachmentState*	pAttachmentVk	= pBlendAttachments + i;
			const BlendAttachmentStateDesc*			pAttachment		= pDesc->BlendState.pBlendAttachmentStates + i;

			pAttachmentVk->blendEnable			= (pAttachment->BlendEnabled) ? VK_TRUE : VK_FALSE;
			pAttachmentVk->colorWriteMask		= ConvertColorComponentMask(pAttachment->RenderTargetComponentsMask);
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
		depthStencilState.depthTestEnable		= VK_TRUE;
		depthStencilState.depthWriteEnable		= VK_TRUE;
		depthStencilState.depthCompareOp		= VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable		= VK_FALSE;
		depthStencilState.minDepthBounds		= 0.0f;
		depthStencilState.maxDepthBounds		= 1.0f;
		depthStencilState.front					= {};
		depthStencilState.back					= {};


		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.flags	= 0;
		vertexInputInfo.pNext	= nullptr;

		TArray<VkVertexInputBindingDescription> bindingDescriptors;
		TArray<VkVertexInputAttributeDescription> attributeDescriptors;
		bindingDescriptors.reserve(pDesc->VertexInputBindingCount);

		if (pDesc->VertexInputBindingCount > 0)
		{
			for (uint32 b = 0; b < pDesc->VertexInputBindingCount; b++)
			{
				const VertexInputBindingDesc* pInputBindingDesc = &pDesc->pVertexInputBindings[b];

				VkVertexInputBindingDescription vkInputBindingDesc = {};
				vkInputBindingDesc.binding		= pInputBindingDesc->Binding;
				vkInputBindingDesc.stride		= pInputBindingDesc->Stride;
				vkInputBindingDesc.inputRate	= ConvertVertexInputRate(pInputBindingDesc->InputRate);
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

			vertexInputInfo.vertexAttributeDescriptionCount = (uint32)attributeDescriptors.size();
			vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptors.data();
			vertexInputInfo.vertexBindingDescriptionCount   = (uint32)bindingDescriptors.size();
			vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptors.data();
		}
		else
		{
			vertexInputInfo.vertexAttributeDescriptionCount = 0;
			vertexInputInfo.pVertexAttributeDescriptions    = nullptr;
			vertexInputInfo.vertexBindingDescriptionCount   = 0;
			vertexInputInfo.pVertexBindingDescriptions      = nullptr;
		}

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.flags         = 0;
		viewportState.pNext         = nullptr;
		viewportState.viewportCount = 1;
		viewportState.pViewports    = nullptr;
		viewportState.scissorCount  = 1;
		viewportState.pScissors     = nullptr;

		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.flags              = 0;
		dynamicState.pNext              = nullptr;
		dynamicState.pDynamicStates     = dynamicStates;
		dynamicState.dynamicStateCount  = 2;

		m_pColorBlendAttachmentStates = DBG_NEW VkPipelineColorBlendAttachmentState[pDesc->BlendAttachmentStateCount];

		for (uint32 i = 0; i < pDesc->BlendAttachmentStateCount; i++)
		{
			const BlendAttachmentState* pBlendAttachmentState = pDesc->pBlendAttachmentStates;

			VkPipelineColorBlendAttachmentState blendAttachment = {};
			blendAttachment.blendEnable			= pBlendAttachmentState->BlendEnabled ? VK_TRUE : VK_FALSE;
			blendAttachment.colorWriteMask		= ConvertColorComponentMask(pBlendAttachmentState->ColorComponentsMask);
			blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachment.colorBlendOp		= VK_BLEND_OP_ADD;
			blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachment.alphaBlendOp		= VK_BLEND_OP_ADD;

			m_pColorBlendAttachmentStates[i] = blendAttachment;
		}

		m_BlendState.pAttachments			= m_pColorBlendAttachmentStates;
		m_BlendState.attachmentCount		= pDesc->BlendAttachmentStateCount;

        const PipelineLayoutVK* pPipelineLayoutVk  = reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
        const RenderPassVK*     pRenderPassVk      = reinterpret_cast<const RenderPassVK*>(pDesc->pRenderPass);
        
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext                  = nullptr;
		pipelineInfo.flags                  = 0;
		pipelineInfo.stageCount             = uint32_t(shaderStagesInfos.size());
		pipelineInfo.pStages                = shaderStagesInfos.data();
		pipelineInfo.pVertexInputState      = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState    = &inputAssembly;
		pipelineInfo.pViewportState         = &viewportState;
		pipelineInfo.pRasterizationState    = &rasterizerState;
		pipelineInfo.pMultisampleState      = &multisamplingState;
		pipelineInfo.pDepthStencilState     = &depthStencilState;
		pipelineInfo.pColorBlendState       = &blendState;
		pipelineInfo.pDynamicState          = &dynamicState;
		pipelineInfo.renderPass             = pRenderPassVk->GetRenderPass();
		pipelineInfo.layout                 = pPipelineLayoutVk->GetPipelineLayout();
		pipelineInfo.subpass                = 0;
		pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex      = -1;

        VkResult result = vkCreateGraphicsPipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
            if (pDesc->pName)
            {
                LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed for %s", pDesc->pName);
            }
            else
            {
                LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed for");
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

	bool GraphicsPipelineStateVK::CreateShaderData(
		std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos, 
		std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps, 
		const GraphicsPipelineStateDesc* pDesc)
	{
		if (pDesc->pMeshShader != nullptr)
		{
			//Mesh Shader
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pMeshShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}

			//Task shader
			if (pDesc->pTaskShader)
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pTaskShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}
		}
		else
		{
			//Vertex Shader
			if (pDesc->pVertexShader != nullptr)
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pVertexShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}
			else
			{
				if (pDesc->pName)
				{
					LOG_ERROR("[GraphicsPipelineStateVK]: Vertex Shader and Mesh Shader can not both be nullptr for %s", pDesc->pName);
				}
				else
				{
					LOG_ERROR("[GraphicsPipelineStateVK]: Vertex Shader and Mesh Shader can not both be nullptr");
				}

				return false;
			}

			//Geometry Shader
			if (pDesc->pGeometryShader != nullptr)
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pGeometryShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}

			//Hull Shader
			if (pDesc->pHullShader != nullptr)
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pHullShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}

			//Domain Shader
			if (pDesc->pDomainShader != nullptr)
			{
				const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pDomainShader);

				VkPipelineShaderStageCreateInfo shaderCreateInfo;
				VkSpecializationInfo shaderSpecializationInfo;
				std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

				pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
				pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

				shaderStagesInfos.push_back(shaderCreateInfo);
				shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
				shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
			}
		}

		//Pixel Shader
		if (pDesc->pPixelShader != nullptr)
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pPixelShader);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);
		}

		return true;
	}
}
