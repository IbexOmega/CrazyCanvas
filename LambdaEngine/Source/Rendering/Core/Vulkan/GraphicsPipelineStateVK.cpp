#include "Log/Log.h"

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
		//Default InputAssembly
		m_InputAssembly.sType                   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_InputAssembly.flags                   = 0;
		m_InputAssembly.pNext                   = nullptr;
		m_InputAssembly.topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		m_InputAssembly.primitiveRestartEnable  = VK_FALSE;

		//Default RasterizerState
		m_RasterizerState.sType                     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_RasterizerState.flags                     = 0;
		m_RasterizerState.pNext                     = nullptr;
		m_RasterizerState.polygonMode               = VK_POLYGON_MODE_FILL;
		m_RasterizerState.lineWidth                 = 1.0f;
		m_RasterizerState.cullMode                  = VK_CULL_MODE_BACK_BIT;
		m_RasterizerState.frontFace                 = VK_FRONT_FACE_CLOCKWISE;
		m_RasterizerState.depthBiasEnable           = VK_FALSE;
		m_RasterizerState.depthClampEnable          = VK_FALSE;
		m_RasterizerState.rasterizerDiscardEnable   = VK_FALSE;
		m_RasterizerState.depthBiasClamp            = 0.0f;
		m_RasterizerState.depthBiasConstantFactor   = 0.0f;
		m_RasterizerState.depthBiasSlopeFactor      = 0.0f;

		//MultisamplingState
		m_MultisamplingState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_MultisamplingState.flags                  = 0;
		m_MultisamplingState.pNext                  = nullptr;
		m_MultisamplingState.alphaToCoverageEnable  = VK_FALSE;
		m_MultisamplingState.alphaToOneEnable       = VK_FALSE;
		m_MultisamplingState.minSampleShading       = 0.0f;
		m_MultisamplingState.sampleShadingEnable    = VK_FALSE;
		m_MultisamplingState.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT;
		m_MultisamplingState.pSampleMask            = nullptr;

		//BlendState
		m_BlendState.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_BlendState.flags              = 0;
		m_BlendState.pNext              = nullptr;
		m_BlendState.logicOpEnable      = VK_FALSE;
		m_BlendState.logicOp            = VK_LOGIC_OP_COPY;
		m_BlendState.pAttachments       = nullptr;
		m_BlendState.attachmentCount    = 0;
		m_BlendState.blendConstants[0]  = 0.0f;
		m_BlendState.blendConstants[1]  = 0.0f;
		m_BlendState.blendConstants[2]  = 0.0f;
		m_BlendState.blendConstants[3]  = 0.0f;

		//DepthstencilStated
		m_DepthStencilState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_DepthStencilState.flags                   = 0;
		m_DepthStencilState.pNext                   = nullptr;
		m_DepthStencilState.depthTestEnable         = VK_TRUE;
		m_DepthStencilState.depthWriteEnable        = VK_TRUE;
		m_DepthStencilState.depthCompareOp          = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_DepthStencilState.depthBoundsTestEnable   = VK_FALSE;
		m_DepthStencilState.stencilTestEnable       = VK_FALSE;
		m_DepthStencilState.minDepthBounds          = 0.0f;
		m_DepthStencilState.maxDepthBounds          = 1.0f; 
		m_DepthStencilState.front                   = {};
		m_DepthStencilState.back                    = {};
	}

	GraphicsPipelineStateVK::~GraphicsPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		SAFEDELETE_ARRAY(m_pColorBlendAttachmentStates);
	}

	bool GraphicsPipelineStateVK::Init(const GraphicsPipelineStateDesc* pDesc)
	{
		 // Define shader stage create infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, pDesc))
		{
			return false;
		}

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
		pipelineInfo.pInputAssemblyState    = &m_InputAssembly;
		pipelineInfo.pViewportState         = &viewportState;
		pipelineInfo.pRasterizationState    = &m_RasterizerState;
		pipelineInfo.pMultisampleState      = &m_MultisamplingState;
		pipelineInfo.pDepthStencilState     = &m_DepthStencilState;
		pipelineInfo.pColorBlendState       = &m_BlendState;
		pipelineInfo.pDynamicState          = &dynamicState;
		pipelineInfo.renderPass             = pRenderPassVk->GetRenderPass();
		pipelineInfo.layout                 = pPipelineLayoutVk->GetPipelineLayout();
		pipelineInfo.subpass                = 0;
		pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex      = -1;

        VkResult result = vkCreateGraphicsPipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
            LOG_VULKAN_ERROR(result, "[GraphicsPipelineStateVK]: vkCreateGraphicsPipelines failed for %s", pDesc->Name.c_str());
            
			return false;
		}
        else
        {
            SetName(pDesc->Name.c_str());

            D_LOG_MESSAGE("[GraphicsPipelineStateVK]: Created Pipeline for %s", pDesc->Name.c_str());
            
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
				LOG_ERROR("[GraphicsPipelineStateVK]: Vertex Shader and Mesh Shader can not both be nullptr for %s", pDesc->Name.c_str());

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
