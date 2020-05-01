#include "Rendering/RenderGraphDescriptionParser.h"

#include "Log/Log.h"

#include <cstdio>
#include <map>
#include <unordered_set>

namespace LambdaEngine
{
	bool RenderGraphDescriptionParser::Parse(
		RenderGraphDesc& desc,
		std::vector<RenderStageDesc>& sortedRenderStageDescriptions,
		std::vector<SynchronizationStageDesc>& sortedSynchronizationStageDescriptions,
		std::vector<PipelineStageDesc>& sortedPipelineStageDescriptions,
		std::vector<RenderStageResourceDesc>& resourceDescriptions)
	{
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>		renderStagesInputAttachments;
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>		renderStagesExternalInputAttachments;
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>		renderStagesOutputAttachments;

		std::unordered_map<std::string, InternalRenderStage>							parsedRenderStages;
		std::unordered_map<std::string, InternalRenderStageAttachment>					parsedInputAttachments;
		std::unordered_map<std::string, InternalRenderStageAttachment>					parsedTemporalInputAttachments;
		std::unordered_map<std::string, InternalRenderStageAttachment>					parsedExternalInputAttachments;
		std::unordered_map<std::string, InternalRenderStageAttachment>					parsedOutputAttachments;

		std::vector<const InternalRenderStage*>										sortedInternalRenderStages;

		if (!SortRenderStagesAttachments(
			desc,
			renderStagesInputAttachments,
			renderStagesExternalInputAttachments,
			renderStagesOutputAttachments,
			resourceDescriptions))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not sort Render Stages Input Attachment for \"%s\"", desc.pName);
			return false;
		}

		if (!ParseInitialStages(
			desc,
			renderStagesInputAttachments,
			renderStagesExternalInputAttachments,
			renderStagesOutputAttachments,
			parsedRenderStages,
			parsedInputAttachments,
			parsedTemporalInputAttachments,
			parsedExternalInputAttachments,
			parsedOutputAttachments))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not parse Render Stages for \"%s\"", desc.pName);
			return false;
		}

		if (!ConnectOutputsToInputs(
			parsedInputAttachments,
			parsedOutputAttachments))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not connect Output Resources to Input Resources for \"%s\"", desc.pName);
			return false;
		}

		if (!WeightRenderStages(
			parsedRenderStages))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not weights render stages for \"%s\"", desc.pName);
			return false;
		}

		if (!SortPipelineStages(
			parsedRenderStages,
			sortedInternalRenderStages,
			sortedRenderStageDescriptions,
			sortedSynchronizationStageDescriptions,
			sortedPipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not sort render stages for \"%s\"", desc.pName);
			return false;
		}

		if (!PruneUnnecessarySynchronizations(
			sortedSynchronizationStageDescriptions,
			sortedPipelineStageDescriptions))
		{
			LOG_ERROR("[RenderGraphDescriptionParser]: Could not prunt unnecessary synchronization for \"%s\"", desc.pName);
			return false;
		}

		if (desc.CreateDebugGraph)
		{
			constexpr bool DECLARE_EXTERNAL_INPUTS = true;
			constexpr bool LINK_EXTERNAL_INPUTS = true;

			if (!WriteGraphViz(
				desc.pName, 
				DECLARE_EXTERNAL_INPUTS, 
				LINK_EXTERNAL_INPUTS,
				parsedRenderStages,
				parsedInputAttachments,
				parsedTemporalInputAttachments,
				parsedExternalInputAttachments,
				parsedOutputAttachments,
				sortedInternalRenderStages,
				sortedRenderStageDescriptions,
				sortedSynchronizationStageDescriptions,
				sortedPipelineStageDescriptions))
			{
				LOG_WARNING("[RenderGraphDescriptionParser]: Could not create GraphViz for \"%s\"", desc.pName);
			}
		}
		
		return true;
	}

	bool RenderGraphDescriptionParser::SortRenderStagesAttachments(
		const RenderGraphDesc& desc, 
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&			renderStagesInputAttachments, 
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&			renderStagesExternalInputAttachments, 
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&			renderStagesOutputAttachments,
		std::vector<RenderStageResourceDesc>&												resourceDescriptions)
	{
		std::unordered_map<std::string, const RenderStageAttachment*> renderStageAttachmentMap;
		std::unordered_map<std::string, const RenderStagePushConstants*> renderStagePushConstantsMap;

		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStageDesc* pRenderStageDesc = &desc.pRenderStages[renderStageIndex];

			std::vector<const RenderStageAttachment*>& renderStageInputAttachments			= renderStagesInputAttachments[pRenderStageDesc->pName];
			std::vector<const RenderStageAttachment*>& renderStageExternalInputAttachments	= renderStagesExternalInputAttachments[pRenderStageDesc->pName];
			std::vector<const RenderStageAttachment*>& renderStageOutputAttachments			= renderStagesOutputAttachments[pRenderStageDesc->pName];

			if (pRenderStageDesc->PushConstants.DataSize > 0)
				renderStagePushConstantsMap[pRenderStageDesc->PushConstants.pName] = &pRenderStageDesc->PushConstants;
			

			for (uint32 a = 0; a < pRenderStageDesc->AttachmentCount; a++)
			{
				const RenderStageAttachment* pAttachment = &pRenderStageDesc->pAttachments[a];

				renderStageAttachmentMap[pAttachment->pName] = pAttachment;

				EAttachmentAccessType accessType = GetAttachmentAccessType(pAttachment->Type);

				switch (accessType)
				{
				case EAttachmentAccessType::INPUT:			renderStageInputAttachments.push_back(pAttachment);				break;
				case EAttachmentAccessType::EXTERNAL_INPUT:	renderStageExternalInputAttachments.push_back(pAttachment);		break;
				case EAttachmentAccessType::OUTPUT:			renderStageOutputAttachments.push_back(pAttachment);			break;
				default:
					{
						LOG_ERROR("[RenderGraphDescriptionParser]: Attachment \"%s\" has unsupported type!", pAttachment->pName);
						return false;
					}
				}
			}
		}

		resourceDescriptions.reserve(renderStageAttachmentMap.size() + renderStagePushConstantsMap.size());

		for (auto it = renderStageAttachmentMap.begin(); it != renderStageAttachmentMap.end(); it++)
		{
			RenderStageResourceDesc resourceDescription = {};
			resourceDescription.Type			= ERenderStageResourceType::ATTACHMENT;
			resourceDescription.pAttachmentDesc = it->second;

			resourceDescriptions.push_back(resourceDescription);
		}

		for (auto it = renderStagePushConstantsMap.begin(); it != renderStagePushConstantsMap.end(); it++)
		{
			RenderStageResourceDesc resourceDescription = {};
			resourceDescription.Type				= ERenderStageResourceType::PUSH_CONSTANTS;
			resourceDescription.pPushConstantsDesc	= it->second;

			resourceDescriptions.push_back(resourceDescription);
		}

		return true;
	}

	bool RenderGraphDescriptionParser::ParseInitialStages(
		const RenderGraphDesc& desc,
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesInputAttachments,
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesExternalInputAttachments,
		std::unordered_map<std::string, std::vector<const RenderStageAttachment*>>&		renderStagesOutputAttachments,
		std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments)
	{
		uint32 globalAttachmentIndex = 0;

		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStageDesc& renderStage = desc.pRenderStages[renderStageIndex];

			auto renderStageIt = parsedRenderStages.find(renderStage.pName);

			if (renderStageIt != parsedRenderStages.end())
			{
				LOG_ERROR("[RenderGraphDescriptionParser]: Multiple Render Stages with same name is not allowed, name: \"%s\"", renderStage.pName);
				return false;
			}

			InternalRenderStage& internalRenderStage = parsedRenderStages[renderStage.pName];
			internalRenderStage.pRenderStage = &renderStage;
			internalRenderStage.GlobalIndex = renderStageIndex;

			std::vector<const RenderStageAttachment*>& renderStageInputAttachments			= renderStagesInputAttachments[renderStage.pName];
			std::vector<const RenderStageAttachment*>& renderStageExternalInputAttachments	= renderStagesExternalInputAttachments[renderStage.pName];
			std::vector<const RenderStageAttachment*>& renderStageOutputAttachments			= renderStagesOutputAttachments[renderStage.pName];

			//Input Attachments
			for (uint32 inputAttachmentIndex = 0; inputAttachmentIndex < renderStageInputAttachments.size(); inputAttachmentIndex++)
			{
				const RenderStageAttachment* pRenderStageInputAttachment = renderStageInputAttachments[inputAttachmentIndex];

				auto inputIt = parsedInputAttachments.find(pRenderStageInputAttachment->pName);
				auto temporalInputIt = parsedTemporalInputAttachments.find(pRenderStageInputAttachment->pName);

				bool isTemporal = IsInputTemporal(renderStageOutputAttachments, pRenderStageInputAttachment);

				if (isTemporal)
				{
					//Temporal
					if (temporalInputIt != parsedTemporalInputAttachments.end())
					{
						inputIt->second.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageInputAttachment);
						internalRenderStage.TemporalInputAttachments.push_back(&temporalInputIt->second);
					}
					else
					{
						InternalRenderStageAttachment& internalTemporalInputAttachment = parsedTemporalInputAttachments[pRenderStageInputAttachment->pName];
						internalTemporalInputAttachment.AttachmentName = pRenderStageInputAttachment->pName;
						internalTemporalInputAttachment.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageInputAttachment);
						internalTemporalInputAttachment.GlobalIndex = globalAttachmentIndex;
						globalAttachmentIndex++;

						internalRenderStage.TemporalInputAttachments.push_back(&internalTemporalInputAttachment);
					}
				}
				else
				{
					//Non-Temporal
					if (inputIt != parsedInputAttachments.end())
					{
						inputIt->second.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageInputAttachment);
						internalRenderStage.InputAttachments.push_back(&inputIt->second);
					}
					else
					{
						InternalRenderStageAttachment& internalInputAttachment = parsedInputAttachments[pRenderStageInputAttachment->pName];
						internalInputAttachment.AttachmentName = pRenderStageInputAttachment->pName;
						internalInputAttachment.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageInputAttachment);
						internalInputAttachment.GlobalIndex = globalAttachmentIndex;
						globalAttachmentIndex++;

						internalRenderStage.InputAttachments.push_back(&internalInputAttachment);
					}
				}
			}

			//External Input Attachments
			for (uint32 externalInputAttachmentIndex = 0; externalInputAttachmentIndex < renderStageExternalInputAttachments.size(); externalInputAttachmentIndex++)
			{
				const RenderStageAttachment* pRenderStageExternalInputAttachment = renderStageExternalInputAttachments[externalInputAttachmentIndex];
				auto externalInputIt = parsedExternalInputAttachments.find(pRenderStageExternalInputAttachment->pName);

				if (externalInputIt != parsedExternalInputAttachments.end())
				{
					externalInputIt->second.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageExternalInputAttachment);
					internalRenderStage.ExternalInputAttachments.push_back(&externalInputIt->second);
				}
				else
				{
					InternalRenderStageAttachment& internalExternalInputAttachment = parsedExternalInputAttachments[pRenderStageExternalInputAttachment->pName];
					internalExternalInputAttachment.AttachmentName = pRenderStageExternalInputAttachment->pName;
					internalExternalInputAttachment.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageExternalInputAttachment);
					internalExternalInputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalRenderStage.ExternalInputAttachments.push_back(&internalExternalInputAttachment);
				}
			}

			//Output Attachments
			for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStageOutputAttachments.size(); outputAttachmentIndex++)
			{
				const RenderStageAttachment* pRenderStageOutputAttachment = renderStageOutputAttachments[outputAttachmentIndex];
				auto outputIt = parsedOutputAttachments.find(pRenderStageOutputAttachment->pName);

				if (outputIt != parsedOutputAttachments.end())
				{
					outputIt->second.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageOutputAttachment);
					internalRenderStage.OutputAttachments.push_back(&outputIt->second);
				}
				else
				{
					InternalRenderStageAttachment& internalOutputAttachment = parsedOutputAttachments[pRenderStageOutputAttachment->pName];
					internalOutputAttachment.AttachmentName = pRenderStageOutputAttachment->pName;
					internalOutputAttachment.RenderStageNameToRenderStageAndAttachment[internalRenderStage.pRenderStage->pName] = std::make_pair(&internalRenderStage, pRenderStageOutputAttachment);
					internalOutputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalRenderStage.OutputAttachments.push_back(&internalOutputAttachment);
				}
			}
		}

		return true;
	}

	bool RenderGraphDescriptionParser::ConnectOutputsToInputs(
		std::unordered_map<std::string, InternalRenderStageAttachment>& parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>& parsedOutputAttachments)
	{
		//Connect Output Attachments to Input Attachments
		for (auto& outputAttachmentPair : parsedOutputAttachments)
		{
			InternalRenderStageAttachment& renderStageOutputAttachment = outputAttachmentPair.second;

			for (auto& inputAttachmentPair : parsedInputAttachments)
			{
				InternalRenderStageAttachment& renderStageInputAttachment = inputAttachmentPair.second;

				if (renderStageInputAttachment.AttachmentName == renderStageOutputAttachment.AttachmentName)
				{
					//Connect Render Stages
					for (auto prevIt = renderStageOutputAttachment.RenderStageNameToRenderStageAndAttachment.begin(); prevIt != renderStageOutputAttachment.RenderStageNameToRenderStageAndAttachment.end(); prevIt++)
					{
						InternalRenderStage* pPreviousRenderStage = prevIt->second.first;

						for (auto nextIt = renderStageInputAttachment.RenderStageNameToRenderStageAndAttachment.begin(); nextIt != renderStageInputAttachment.RenderStageNameToRenderStageAndAttachment.end(); nextIt++)
						{
							InternalRenderStage* pNextRenderStage = nextIt->second.first;

							if (AreRenderStagesRelated(pNextRenderStage, pPreviousRenderStage))
							{
								LOG_ERROR("[RenderGraphDescriptionParser]: Cyclic Render Stage dependency detected between \"%s\" and \"%s\"!", pPreviousRenderStage->pRenderStage->pName, pNextRenderStage->pRenderStage->pName);
								return false;
							}

							pPreviousRenderStage->ChildRenderStages.insert(pNextRenderStage);
							pNextRenderStage->ParentRenderStages.insert(pPreviousRenderStage);
						}
					}

					renderStageInputAttachment.pConnectedAttachment = &renderStageOutputAttachment;
					renderStageOutputAttachment.pConnectedAttachment = &renderStageInputAttachment;
					break;
				}
			}
		}

		bool result = true;
		
		for (auto& inputAttachmentPair : parsedInputAttachments)
		{
			InternalRenderStageAttachment& renderStageInputAttachment = inputAttachmentPair.second;

			if (renderStageInputAttachment.pConnectedAttachment == nullptr)
			{
				result = false;
				LOG_ERROR("[RenderGraphDescriptionParser]: Input Attachment \"%s\" has not connected output!", renderStageInputAttachment.AttachmentName.c_str());
			}
		}

		return result;
	}

	bool RenderGraphDescriptionParser::WeightRenderStages(std::unordered_map<std::string, InternalRenderStage>& parsedRenderStages)
	{
		for (auto renderStageIt = parsedRenderStages.begin(); renderStageIt != parsedRenderStages.end(); renderStageIt++)
		{
			RecursivelyWeightAncestors(&renderStageIt->second);
		}

		return true;
	}

	void RenderGraphDescriptionParser::RecursivelyWeightAncestors(InternalRenderStage* pRenderStage)
	{
		for (InternalRenderStage* pParentRenderStage : pRenderStage->ParentRenderStages)
		{
			pParentRenderStage->Weight++;
			RecursivelyWeightAncestors(pParentRenderStage);
		}
	}

	bool RenderGraphDescriptionParser::SortPipelineStages(
		std::unordered_map<std::string, InternalRenderStage>&	parsedRenderStages,
		std::vector<const InternalRenderStage*>&				sortedInternalRenderStages,
		std::vector<RenderStageDesc>&							sortedRenderStages,
		std::vector<SynchronizationStageDesc>&					sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&							sortedPipelineStages)
	{
		std::multimap<uint32, const InternalRenderStage*> weightedRenderStageMap;

		for (auto renderStageIt = parsedRenderStages.begin(); renderStageIt != parsedRenderStages.end(); renderStageIt++)
		{
			weightedRenderStageMap.insert(std::make_pair(renderStageIt->second.Weight, &renderStageIt->second));
		}

		std::unordered_map<std::string, std::pair<const RenderStageAttachment*, EPipelineStateType>> finalStateOfAttachmentsFromPreviousFrame;

		//Store this, because it should be last, not first
		std::vector<AttachmentSynchronizationDesc> endSynchronizations;
		bool endSynchronizationStageValid = false;

		for (auto sortedRenderStageIt = weightedRenderStageMap.begin(); sortedRenderStageIt != weightedRenderStageMap.end(); sortedRenderStageIt++)
		{
			sortedInternalRenderStages.push_back(sortedRenderStageIt->second);

			const RenderStageDesc* pSourceRenderStage = sortedRenderStageIt->second->pRenderStage;

			std::string renderStageName = pSourceRenderStage->pName;

			RenderStageDesc renderStage = {};
			renderStage.pName							= pSourceRenderStage->pName;
			renderStage.pAttachments					= pSourceRenderStage->pAttachments;
			renderStage.AttachmentCount					= pSourceRenderStage->AttachmentCount;
			renderStage.PushConstants					= pSourceRenderStage->PushConstants;

			renderStage.PipelineType					= pSourceRenderStage->PipelineType;
			
			switch (renderStage.PipelineType)
			{
				case EPipelineStateType::GRAPHICS:
				{
					renderStage.GraphicsPipeline.pGraphicsDesc			= pSourceRenderStage->GraphicsPipeline.pGraphicsDesc;
					renderStage.GraphicsPipeline.DrawType				= pSourceRenderStage->GraphicsPipeline.DrawType;
					renderStage.GraphicsPipeline.pIndexBufferName		= pSourceRenderStage->GraphicsPipeline.pIndexBufferName;
					renderStage.GraphicsPipeline.pMeshIndexBufferName	= pSourceRenderStage->GraphicsPipeline.pMeshIndexBufferName;
					break;
				}
				case EPipelineStateType::COMPUTE:
				{
					renderStage.ComputePipeline.pComputeDesc = pSourceRenderStage->ComputePipeline.pComputeDesc;
					break;
				}
				case EPipelineStateType::RAY_TRACING:
				{
					renderStage.RayTracingPipeline.pRayTracingDesc = pSourceRenderStage->RayTracingPipeline.pRayTracingDesc;
					break;
				}
			}		

			sortedRenderStages.push_back(renderStage);

			PipelineStageDesc renderPipelineStage = {};
			renderPipelineStage.Type = EPipelineStageType::RENDER;
			renderPipelineStage.StageIndex = uint32(sortedRenderStages.size() - 1);
			sortedPipelineStages.push_back(renderPipelineStage);

			SynchronizationStageDesc synchronizationStage = {};

			//Create External Input Synchronizations
			{
				for (const InternalRenderStageAttachment* pExternalInputAttachment : sortedRenderStageIt->second->ExternalInputAttachments)
				{
					AttachmentSynchronizationDesc attachmentSynchronization = {};
					attachmentSynchronization.Type					= EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ;
					attachmentSynchronization.ToQueueOwner			= sortedRenderStageIt->second->pRenderStage->PipelineType;
					attachmentSynchronization.ToAttachment			= *(pExternalInputAttachment->RenderStageNameToRenderStageAndAttachment.find(renderStageName)->second.second);

					synchronizationStage.Synchronizations.push_back(attachmentSynchronization);
				}
			}

			//Create Output to Input Synchronizations
			{
				for (const InternalRenderStageAttachment* pInputAttachment : sortedRenderStageIt->second->InputAttachments)
				{
					AttachmentSynchronizationDesc attachmentSynchronization = {};
					attachmentSynchronization.Type				= EAttachmentSynchronizationType::TRANSITION_FOR_READ;
					attachmentSynchronization.ToQueueOwner		= sortedRenderStageIt->second->pRenderStage->PipelineType;
					attachmentSynchronization.FromAttachment	= *(pInputAttachment->pConnectedAttachment->RenderStageNameToRenderStageAndAttachment.begin()->second.second); //Assume only one write render stage
					attachmentSynchronization.ToAttachment		= *(pInputAttachment->RenderStageNameToRenderStageAndAttachment.find(renderStageName)->second.second);

					synchronizationStage.Synchronizations.push_back(attachmentSynchronization);

					auto finalStateOfAttachmentIt = finalStateOfAttachmentsFromPreviousFrame.find(pInputAttachment->AttachmentName);

					if (finalStateOfAttachmentIt == finalStateOfAttachmentsFromPreviousFrame.end())
					{
						finalStateOfAttachmentsFromPreviousFrame[pInputAttachment->AttachmentName] = 
							std::make_pair(pInputAttachment->RenderStageNameToRenderStageAndAttachment.find(renderStageName)->second.second, sortedRenderStageIt->second->pRenderStage->PipelineType);
					}
				}
			}

			//Create Input (From Prev Frame) to Output Synchronizations, but push the synchronizations to the final stage
			{
				for (const InternalRenderStageAttachment* pOutputAttachment : sortedRenderStageIt->second->OutputAttachments)
				{
					//Check if this resource is back buffer and this is not graphics queue
					
					if (pSourceRenderStage->PipelineType != EPipelineStateType::GRAPHICS)
					{
						if (strcmp(pOutputAttachment->AttachmentName.c_str(), RENDER_GRAPH_BACK_BUFFER_ATTACHMENT) == 0)
						{
							RenderStageAttachment backBufferToAttachment = *(pOutputAttachment->RenderStageNameToRenderStageAndAttachment.find(renderStageName)->second.second);

							RenderStageAttachment backBufferFromAttachment = {};
							backBufferFromAttachment.pName				= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
							backBufferFromAttachment.Type				= EAttachmentType::NONE;
							backBufferFromAttachment.ShaderStages		= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
							backBufferFromAttachment.SubResourceCount	= backBufferToAttachment.SubResourceCount;

							AttachmentSynchronizationDesc backBufferPreSynchronization = {};
							backBufferPreSynchronization.Type					= EAttachmentSynchronizationType::TRANSITION_FOR_WRITE;
							backBufferPreSynchronization.FromQueueOwner			= EPipelineStateType::GRAPHICS;
							backBufferPreSynchronization.ToQueueOwner			= sortedRenderStageIt->second->pRenderStage->PipelineType;
							backBufferPreSynchronization.FromAttachment			= backBufferFromAttachment;
							backBufferPreSynchronization.ToAttachment			= backBufferToAttachment;

							synchronizationStage.Synchronizations.push_back(backBufferPreSynchronization);

							AttachmentSynchronizationDesc backBufferPostSynchronization = {};
							backBufferPostSynchronization.Type					= EAttachmentSynchronizationType::TRANSITION_FOR_READ;
							backBufferPostSynchronization.FromQueueOwner		= sortedRenderStageIt->second->pRenderStage->PipelineType;
							backBufferPostSynchronization.ToQueueOwner			= EPipelineStateType::GRAPHICS; 
							backBufferPostSynchronization.FromAttachment		= backBufferToAttachment;
							backBufferPostSynchronization.ToAttachment			= backBufferFromAttachment;

							endSynchronizations.push_back(backBufferPostSynchronization);

							endSynchronizationStageValid = true;
							continue;
						}
					}

					auto finalStateOfAttachmentIt = finalStateOfAttachmentsFromPreviousFrame.find(pOutputAttachment->AttachmentName);

					//If final state not found this is the only stage that uses the attachment -> no synchronization required
					if (finalStateOfAttachmentIt == finalStateOfAttachmentsFromPreviousFrame.end())
					{
						continue;
					}

					const RenderStageAttachment* pAttachment = pOutputAttachment->RenderStageNameToRenderStageAndAttachment.find(renderStageName)->second.second;

					//If the attachment is a renderpass attachment, the transition will be handled by a renderpass...
					if (pAttachment->Type != EAttachmentType::OUTPUT_COLOR && pAttachment->Type != EAttachmentType::OUTPUT_DEPTH_STENCIL)
					{
						AttachmentSynchronizationDesc attachmentSynchronization = {};
						attachmentSynchronization.Type				= EAttachmentSynchronizationType::TRANSITION_FOR_WRITE;
						attachmentSynchronization.FromQueueOwner	= finalStateOfAttachmentIt->second.second;
						attachmentSynchronization.ToQueueOwner		= sortedRenderStageIt->second->pRenderStage->PipelineType;
						attachmentSynchronization.FromAttachment	= *finalStateOfAttachmentIt->second.first;
						attachmentSynchronization.ToAttachment		= *pAttachment;

						//synchronizationStage.Synchronizations.push_back(attachmentSynchronization);
						endSynchronizations.push_back(attachmentSynchronization);
						endSynchronizationStageValid = true;
					}
					//... unless it also belonged to another queue
					else if (finalStateOfAttachmentIt->second.second != sortedRenderStageIt->second->pRenderStage->PipelineType)
					{
						AttachmentSynchronizationDesc attachmentSynchronization = {};
						attachmentSynchronization.Type				= EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE;
						attachmentSynchronization.FromQueueOwner	= finalStateOfAttachmentIt->second.second;
						attachmentSynchronization.ToQueueOwner		= sortedRenderStageIt->second->pRenderStage->PipelineType;
						attachmentSynchronization.FromAttachment	= *finalStateOfAttachmentIt->second.first;
						attachmentSynchronization.ToAttachment		= *pAttachment;

						//synchronizationStage.Synchronizations.push_back(attachmentSynchronization);
						endSynchronizations.push_back(attachmentSynchronization);
						endSynchronizationStageValid = true;
					}
				}
			}

			//Push Synchronization Stage to Graph
			if (synchronizationStage.Synchronizations.size() > 0)
			{
				//Check if this is the first Render Stage
				if (strcmp(sortedRenderStageIt->second->pRenderStage->pName, weightedRenderStageMap.rbegin()->second->pRenderStage->pName) != 0)
				{
					sortedSynchronizationStages.push_back(synchronizationStage);

					PipelineStageDesc synchronizationPipelineStage = {};
					synchronizationPipelineStage.Type = EPipelineStageType::SYNCHRONIZATION;
					synchronizationPipelineStage.StageIndex = uint32(sortedSynchronizationStages.size() - 1);
					sortedPipelineStages.push_back(synchronizationPipelineStage);
				}
				else
				{
					for (AttachmentSynchronizationDesc& desc : synchronizationStage.Synchronizations)
					{
						endSynchronizations.push_back(desc);
					}

					endSynchronizationStageValid = true;
				}
			}
		}

		std::reverse(sortedRenderStages.begin(), sortedRenderStages.end());
		std::reverse(sortedSynchronizationStages.begin(), sortedSynchronizationStages.end());

		std::reverse(sortedInternalRenderStages.begin(), sortedInternalRenderStages.end());
		std::reverse(sortedPipelineStages.begin(), sortedPipelineStages.end());


		//Set Stage Indices and update External Input Attachments From Attachemnt
		std::unordered_map<std::string, const RenderStageAttachment*> finalStateOfAttachments;

		for (auto sortedPipelineStagesIt = sortedPipelineStages.begin(); sortedPipelineStagesIt != sortedPipelineStages.end(); sortedPipelineStagesIt++)
		{
			if (sortedPipelineStagesIt->Type == EPipelineStageType::RENDER)
			{
				sortedPipelineStagesIt->StageIndex = uint32(sortedRenderStages.size() - sortedPipelineStagesIt->StageIndex - 1);

				const InternalRenderStage* pInternalRenderStage = sortedInternalRenderStages[sortedPipelineStagesIt->StageIndex];

				for (const InternalRenderStageAttachment* pExternalInputAttachment : pInternalRenderStage->ExternalInputAttachments)
				{
					finalStateOfAttachments[pExternalInputAttachment->AttachmentName] = pExternalInputAttachment->RenderStageNameToRenderStageAndAttachment.find(pInternalRenderStage->pRenderStage->pName)->second.second;
				}
			}
			else if (sortedPipelineStagesIt->Type == EPipelineStageType::SYNCHRONIZATION)
			{
				sortedPipelineStagesIt->StageIndex = uint32(sortedSynchronizationStages.size() - sortedPipelineStagesIt->StageIndex - 1);

				SynchronizationStageDesc& synchronizationStageDesc = sortedSynchronizationStages[sortedPipelineStagesIt->StageIndex];

				for (AttachmentSynchronizationDesc& synchronization : synchronizationStageDesc.Synchronizations)
				{
					if (synchronization.FromAttachment.Type == EAttachmentType::NONE)
					{
						auto it = finalStateOfAttachments.find(synchronization.ToAttachment.pName);

						if (it != finalStateOfAttachments.end())
						{
							synchronization.FromAttachment = *finalStateOfAttachments[synchronization.ToAttachment.pName];
						}
					}
				}
			}
		}

		if (endSynchronizationStageValid)
		{
			SynchronizationStageDesc toGraphicsSynchronizationStage;
			SynchronizationStageDesc toComputeSynchronizationStage;

			for (AttachmentSynchronizationDesc& synchronization : endSynchronizations)
			{
				if (synchronization.FromAttachment.Type == EAttachmentType::NONE)
				{
					auto it = finalStateOfAttachments.find(synchronization.ToAttachment.pName);

					if (it != finalStateOfAttachments.end())
					{
						synchronization.FromAttachment = *finalStateOfAttachments[synchronization.ToAttachment.pName];
					}
				}

				if (synchronization.ToQueueOwner == EPipelineStateType::GRAPHICS)
				{
					toGraphicsSynchronizationStage.Synchronizations.push_back(synchronization);
				}
				else if (synchronization.ToQueueOwner == EPipelineStateType::COMPUTE || synchronization.ToQueueOwner == EPipelineStateType::RAY_TRACING)
				{
					toComputeSynchronizationStage.Synchronizations.push_back(synchronization);
				}
			}

			if (toGraphicsSynchronizationStage.Synchronizations.size() > 0)
			{
				sortedSynchronizationStages.push_back(toGraphicsSynchronizationStage);

				PipelineStageDesc synchronizationPipelineStage = {};
				synchronizationPipelineStage.Type			= EPipelineStageType::SYNCHRONIZATION;
				synchronizationPipelineStage.StageIndex		= uint32(sortedSynchronizationStages.size() - 1);
				sortedPipelineStages.push_back(synchronizationPipelineStage);
			}

			if (toComputeSynchronizationStage.Synchronizations.size() > 0)
			{
				sortedSynchronizationStages.push_back(toComputeSynchronizationStage);

				PipelineStageDesc synchronizationPipelineStage = {};
				synchronizationPipelineStage.Type			= EPipelineStageType::SYNCHRONIZATION;
				synchronizationPipelineStage.StageIndex		= uint32(sortedSynchronizationStages.size() - 1);
				sortedPipelineStages.push_back(synchronizationPipelineStage);
			}
		}

		return true;
	}

	bool RenderGraphDescriptionParser::PruneUnnecessarySynchronizations(
		std::vector<SynchronizationStageDesc>&	sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&			sortedPipelineStages)
	{
		std::unordered_map<std::string, AttachmentSynchronizationDesc*> firstEncounterOfAttachmentSynchronizations;
		std::unordered_map<std::string, std::pair<EAttachmentState, EPipelineStateType>> transitionedResourceStates;

		for (auto synchronizationStageIt = sortedSynchronizationStages.begin(); synchronizationStageIt != sortedSynchronizationStages.end(); synchronizationStageIt++)
		{
			for (auto attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.begin(); attachmentSynchronizationIt != synchronizationStageIt->Synchronizations.end(); )
			{
				const char* pAttachmentName = attachmentSynchronizationIt->ToAttachment.pName;
				auto firstEncounterOfAttachmentSynchronizationIt = firstEncounterOfAttachmentSynchronizations.find(pAttachmentName);

				//Store first encounter in graph of each attachment synchronization, their from queue is not known until we have traversed the entire graph to the end
				if (firstEncounterOfAttachmentSynchronizationIt == firstEncounterOfAttachmentSynchronizations.end())
				{
					firstEncounterOfAttachmentSynchronizations[pAttachmentName] = &(*attachmentSynchronizationIt);
				}

				auto transitionedResourceStateIt = transitionedResourceStates.find(pAttachmentName);

				if (attachmentSynchronizationIt->Type != EAttachmentSynchronizationType::TRANSITION_FOR_WRITE && attachmentSynchronizationIt->Type != EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE && transitionedResourceStateIt != transitionedResourceStates.end())
				{
					//If its state already is read, or the access type will be handled by a renderpass, it might just be a queue ownership change
					if (transitionedResourceStateIt->second.first == EAttachmentState::READ || attachmentSynchronizationIt->FromAttachment.Type == EAttachmentType::OUTPUT_COLOR || attachmentSynchronizationIt->FromAttachment.Type == EAttachmentType::OUTPUT_DEPTH_STENCIL)
					{
						if (attachmentSynchronizationIt->ToQueueOwner != transitionedResourceStateIt->second.second)
						{
							attachmentSynchronizationIt->FromQueueOwner = transitionedResourceStateIt->second.second;
							attachmentSynchronizationIt->Type = EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ;
						}
						else
						{
							attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.erase(attachmentSynchronizationIt);
							continue;
						}
					}
					else
					{
						attachmentSynchronizationIt->FromQueueOwner = transitionedResourceStateIt->second.second;
					}
				}

				EAttachmentState resultState = EAttachmentState::NONE;

				switch (attachmentSynchronizationIt->Type)
				{
				case EAttachmentSynchronizationType::NONE:
					resultState = transitionedResourceStateIt != transitionedResourceStates.end() ? transitionedResourceStateIt->second.first : EAttachmentState::NONE;
					break;
				case EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ:
				case EAttachmentSynchronizationType::TRANSITION_FOR_READ:
					resultState = EAttachmentState::READ;
					break;
				case EAttachmentSynchronizationType::TRANSITION_FOR_WRITE:
				case EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE:
					resultState = EAttachmentState::WRITE;
					break;
				}

				transitionedResourceStates[pAttachmentName] = std::make_pair(resultState, attachmentSynchronizationIt->ToQueueOwner);
				attachmentSynchronizationIt++;
			}
		}

		//Update first encounters, is this really necessary? Shouldn't the previous step SortPipelineStages, where we traverse backwards fix first encounters?
		for (auto firstEncounterSynchronizationIt = firstEncounterOfAttachmentSynchronizations.begin(); firstEncounterSynchronizationIt != firstEncounterOfAttachmentSynchronizations.end(); firstEncounterSynchronizationIt++)
		{
			firstEncounterSynchronizationIt->second->FromQueueOwner = transitionedResourceStates[firstEncounterSynchronizationIt->first].second;
		}

		//Pruning of OWNERSHIP_CHANGE from x -> x and empty synchronization stages
		for (auto synchronizationStageIt = sortedSynchronizationStages.begin(); synchronizationStageIt != sortedSynchronizationStages.end();)
		{
			for (auto attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.begin(); attachmentSynchronizationIt != synchronizationStageIt->Synchronizations.end(); )
			{
				//Remove unnecessary OWNERSHIP_CHANGEs
				if (attachmentSynchronizationIt->Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ || attachmentSynchronizationIt->Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE)
				{
					if (attachmentSynchronizationIt->FromQueueOwner == attachmentSynchronizationIt->ToQueueOwner)
					{
						attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.erase(attachmentSynchronizationIt);
						continue;
					}
				}
				//Remove TRANSITION_FOR_READ synchronizations that get handled by Renderpass
				else if (attachmentSynchronizationIt->Type == EAttachmentSynchronizationType::TRANSITION_FOR_READ)
				{
					if (attachmentSynchronizationIt->FromAttachment.Type == EAttachmentType::OUTPUT_COLOR || attachmentSynchronizationIt->FromAttachment.Type == EAttachmentType::OUTPUT_DEPTH_STENCIL)
					{
						if (attachmentSynchronizationIt->FromQueueOwner == attachmentSynchronizationIt->ToQueueOwner)
						{
							attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.erase(attachmentSynchronizationIt);
							continue;
						}
						else
						{
							attachmentSynchronizationIt->Type = EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ;
						}
					}
				}

				attachmentSynchronizationIt++;
			}

			//Remove Synchronization stage if empty
			if (!synchronizationStageIt->Synchronizations.empty())
			{
				synchronizationStageIt++;
			}
			else
			{
				//Remove Pipeline Stage
				bool stageRemoved = false;
				for (auto pipelineStageIt = sortedPipelineStages.begin(); pipelineStageIt != sortedPipelineStages.end();)
				{
					if (pipelineStageIt->Type == EPipelineStageType::SYNCHRONIZATION)
					{
						if (!stageRemoved)
						{
							if (pipelineStageIt->StageIndex == (synchronizationStageIt - sortedSynchronizationStages.begin()))
							{
								pipelineStageIt = sortedPipelineStages.erase(pipelineStageIt);
								stageRemoved = true;
								continue;
							}
						}
						else
						{
							pipelineStageIt->StageIndex--;
						}
					}

					pipelineStageIt++;

				}

				synchronizationStageIt = sortedSynchronizationStages.erase(synchronizationStageIt);
			}
		}

		return true;
	}

	bool RenderGraphDescriptionParser::IsInputTemporal(const std::vector<const RenderStageAttachment*>& renderStageOutputAttachments, const RenderStageAttachment* pInputAttachment)
	{
		for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStageOutputAttachments.size(); outputAttachmentIndex++)
		{
			const RenderStageAttachment* pRenderStageOutputAttachment = renderStageOutputAttachments[outputAttachmentIndex];

			if (strcmp(pInputAttachment->pName, pRenderStageOutputAttachment->pName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool RenderGraphDescriptionParser::AreRenderStagesRelated(const InternalRenderStage* pRenderStageAncestor, const InternalRenderStage* pRenderStageDescendant)
	{
		for (const InternalRenderStage* pChildRenderStage : pRenderStageAncestor->ChildRenderStages)
		{
			if (pChildRenderStage == pRenderStageDescendant)
				return true;

			if (AreRenderStagesRelated(pChildRenderStage, pRenderStageDescendant))
				return true;
		}

		return false;
	}

	bool RenderGraphDescriptionParser::WriteGraphViz(
		const char* pName, 
		bool declareExternalInputs, 
		bool linkExternalInputs,
		std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStageDesc>&														sortedRenderStages,
		std::vector<SynchronizationStageDesc>&												sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&														sortedPipelineStages)
	{
		char renderGraphNameBuffer[256];
		char filepathBuffer[256];

		{
			strcpy(renderGraphNameBuffer, pName);

			SanitizeString(renderGraphNameBuffer, (uint32)strlen(renderGraphNameBuffer));

			strcpy(filepathBuffer, "rs_ord_");
			strcat(filepathBuffer, renderGraphNameBuffer);
			strcat(filepathBuffer, ".dot");

			FILE* pGraphVizFile = fopen(filepathBuffer, "w");

			if (pGraphVizFile == nullptr)
			{
				LOG_WARNING("[ResourceDevice]: Failed to load file \"%s\"", filepathBuffer);
				return false;
			}

			fputs("digraph G {\n", pGraphVizFile);
			fputs("compound=true;\n", pGraphVizFile);
			fputs("\trankdir = LR;\n", pGraphVizFile);
			fputs("\tsplines=polyline\n", pGraphVizFile);

			WriteGraphVizPipelineStages(
				pGraphVizFile,
				parsedRenderStages,
				parsedInputAttachments,
				parsedTemporalInputAttachments,
				parsedExternalInputAttachments,
				parsedOutputAttachments,
				sortedInternalRenderStages,
				sortedRenderStages,
				sortedSynchronizationStages,
				sortedPipelineStages);

			fputs("}", pGraphVizFile);

			fclose(pGraphVizFile);

			char outputCommand[1024];
			strcpy(outputCommand, "D:\\\\Graphviz\\\\bin\\\\dot \"rs_ord_");
			strcat(outputCommand, renderGraphNameBuffer);
			strcat(outputCommand, ".dot\" -O\"");
			strcat(outputCommand, renderGraphNameBuffer);
			strcat(outputCommand, ".pdf\" -T");
			strcat(outputCommand, "pdf");

			system(outputCommand);
		}

		{
			strcpy(renderGraphNameBuffer, pName);

			SanitizeString(renderGraphNameBuffer, (uint32)strlen(renderGraphNameBuffer));

			strcpy(filepathBuffer, "complete_");
			strcat(filepathBuffer, renderGraphNameBuffer);
			strcat(filepathBuffer, ".dot");

			SanitizeString(filepathBuffer, (uint32)strlen(filepathBuffer));

			FILE* pGraphVizFile = fopen(filepathBuffer, "w");

			if (pGraphVizFile == nullptr)
			{
				LOG_WARNING("[ResourceDevice]: Failed to load file \"%s\"", filepathBuffer);
				return false;
			}

			fputs("digraph G {\n", pGraphVizFile);
			fputs("\trankdir = LR;\n", pGraphVizFile);
			fputs("\tsplines=polyline\n", pGraphVizFile);

			WriteGraphVizCompleteDeclarations(
				pGraphVizFile, 
				declareExternalInputs,
				parsedRenderStages,
				parsedInputAttachments,
				parsedTemporalInputAttachments,
				parsedExternalInputAttachments,
				parsedOutputAttachments,
				sortedInternalRenderStages,
				sortedRenderStages,
				sortedSynchronizationStages,
				sortedPipelineStages);
			
			WriteGraphVizCompleteDefinitions(
				pGraphVizFile, 
				declareExternalInputs, 
				linkExternalInputs,
				parsedRenderStages,
				parsedInputAttachments,
				parsedTemporalInputAttachments,
				parsedExternalInputAttachments,
				parsedOutputAttachments,
				sortedInternalRenderStages,
				sortedRenderStages,
				sortedSynchronizationStages,
				sortedPipelineStages);

			fputs("}", pGraphVizFile);

			fclose(pGraphVizFile);

			char outputCommand[1024];
			strcpy(outputCommand, "D:\\\\Graphviz\\\\bin\\\\dot \"complete_");
			strcat(outputCommand, renderGraphNameBuffer);
			strcat(outputCommand, ".dot\" -O\"");
			strcat(outputCommand, renderGraphNameBuffer);
			strcat(outputCommand, ".pdf\" -T");
			strcat(outputCommand, "pdf");

			system(outputCommand);
		}

		return true;
	}

	void RenderGraphDescriptionParser::WriteGraphVizPipelineStages(
		FILE* pFile,
		std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStageDesc>&													sortedRenderStages,
		std::vector<SynchronizationStageDesc>&											sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&													sortedPipelineStages)
	{
		UNREFERENCED_VARIABLE(parsedRenderStages);
		UNREFERENCED_VARIABLE(parsedInputAttachments);
		UNREFERENCED_VARIABLE(parsedTemporalInputAttachments);
		UNREFERENCED_VARIABLE(parsedExternalInputAttachments);
		UNREFERENCED_VARIABLE(parsedOutputAttachments);

		char fileOutputBuffer[256];
		std::vector<std::string> edgesBuffer;
		
		{
			char lastNode[256];
			char lastCluster[256];
			bool connectToPrevious = false;
			bool previousIsSynchronization = false;

			char pipelineStageIndexBuffer[32];
			char renderStageNameBuffer[32];
			char renderStageIndexBuffer[32];
			char attachmentSynchronizationNameBuffer[32];
			char synchronizationStageIndexBuffer[32];
			strcpy(renderStageNameBuffer, "rs");
			strcpy(attachmentSynchronizationNameBuffer, "as");

			uint32 pipelineStageExecutionIndex = 0;
			uint32 renderStageIndex = 0;
			uint32 synchronizationStageIndex = 0;
			uint32 attachmentSynchronizationIndex = 0;

			for (const PipelineStageDesc& pipelineStage : sortedPipelineStages)
			{
				if (pipelineStage.Type == EPipelineStageType::RENDER)
				{
					const RenderStageDesc& renderStage = sortedRenderStages[pipelineStage.StageIndex];

					sprintf(pipelineStageIndexBuffer, "%u", pipelineStageExecutionIndex++);
					sprintf(renderStageIndexBuffer, "%u", renderStageIndex++);

					strcpy(renderStageNameBuffer + 2, renderStageIndexBuffer);

					strcpy(fileOutputBuffer, "\t");
					strcat(fileOutputBuffer, renderStageNameBuffer);
					strcat(fileOutputBuffer, " [shape = box, label = \"Pipeline Stage ");
					strcat(fileOutputBuffer, pipelineStageIndexBuffer);
					strcat(fileOutputBuffer, "\\nRender Stage ");
					strcat(fileOutputBuffer, renderStageIndexBuffer);
					strcat(fileOutputBuffer, "\\n");
					strcat(fileOutputBuffer, renderStage.pName);
					strcat(fileOutputBuffer, "\"];\n");

					fputs(fileOutputBuffer, pFile);

					if (connectToPrevious)
					{
						char edge[256];

						strcpy(edge, "\t");
						strcat(edge, lastNode);
						strcat(edge, " -> ");
						strcat(edge, renderStageNameBuffer);

						if (previousIsSynchronization)
						{
							strcat(edge, "[ltail=");
							strcat(edge, lastCluster);
							strcat(edge, "];\n");
						}
						else
						{
							strcat(edge, "\n");
						}

						edgesBuffer.push_back(edge);
					}

					connectToPrevious = true;
					previousIsSynchronization = false;

					strcpy(lastNode, renderStageNameBuffer);
				}
				else if (pipelineStage.Type == EPipelineStageType::SYNCHRONIZATION)
				{
					const SynchronizationStageDesc& synchronizationStage = sortedSynchronizationStages[pipelineStage.StageIndex];

					sprintf(pipelineStageIndexBuffer, "%u", pipelineStageExecutionIndex++);
					sprintf(synchronizationStageIndexBuffer, "%u", synchronizationStageIndex++);

					strcpy(fileOutputBuffer, "\tsubgraph cluster");
					strcat(fileOutputBuffer, synchronizationStageIndexBuffer);
					strcat(fileOutputBuffer, "{\n");
					strcat(fileOutputBuffer, "\t\tnode [style=filled,color=white];\n");
					strcat(fileOutputBuffer, "\t\tstyle = filled;\n");
					strcat(fileOutputBuffer, "\t\tcolor = lightgrey;\n");
					strcat(fileOutputBuffer, "\t\tlabel = \"Pipeline Stage ");
					strcat(fileOutputBuffer, pipelineStageIndexBuffer);
					strcat(fileOutputBuffer, "\\nSynchronization Stage ");
					strcat(fileOutputBuffer, synchronizationStageIndexBuffer);
					strcat(fileOutputBuffer, "\";\n");
					fputs(fileOutputBuffer, pFile);

					strcpy(lastCluster, "cluster");
					strcat(lastCluster, synchronizationStageIndexBuffer);

					for (const AttachmentSynchronizationDesc& synchronization : synchronizationStage.Synchronizations)
					{
						sprintf(attachmentSynchronizationNameBuffer + 2, "%u", attachmentSynchronizationIndex++);

						strcpy(fileOutputBuffer, "\t\t");
						strcat(fileOutputBuffer, attachmentSynchronizationNameBuffer);
						strcat(fileOutputBuffer, " [shape=box,label=\"");

						if (synchronization.Type == EAttachmentSynchronizationType::TRANSITION_FOR_READ)
						{
							strcat(fileOutputBuffer, synchronization.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--TRANSITION FOR READ--\\n");
						}
						else if (synchronization.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_READ)
						{
							strcat(fileOutputBuffer, synchronization.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--OWNERSHIP CHANGE READ--\\n");
						}
						else if (synchronization.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE_WRITE)
						{
							strcat(fileOutputBuffer, synchronization.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--OWNERSHIP CHANGE WRITE--\\n");
						}
						else if (synchronization.Type == EAttachmentSynchronizationType::TRANSITION_FOR_WRITE)
						{
							strcat(fileOutputBuffer, synchronization.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--TRANSITION FOR WRITE--\\n");
						}

						ConcatPipelineStateToString(fileOutputBuffer, synchronization.FromQueueOwner);
						strcat(fileOutputBuffer, " -> ");
						ConcatPipelineStateToString(fileOutputBuffer, synchronization.ToQueueOwner);
						
						strcat(fileOutputBuffer, "\"];\n");
						fputs(fileOutputBuffer, pFile);
					}

					fputs("\t}\n", pFile);

					if (connectToPrevious)
					{
						char edge[256];

						strcpy(edge, "\t");
						strcat(edge, lastNode);
						strcat(edge, " -> ");
						strcat(edge, attachmentSynchronizationNameBuffer);

						if (!previousIsSynchronization)
						{
							strcat(edge, "[lhead=");
							strcat(edge, lastCluster);
							strcat(edge, "];\n");
						}
						else
						{
							strcat(edge, "\n");
						}

						edgesBuffer.push_back(edge);
					}

					connectToPrevious = true;
					previousIsSynchronization = true;

					strcpy(lastNode, attachmentSynchronizationNameBuffer);
				}
			}
		}

		{
			char parentRenderStageNameBuffer[32];
			char childRenderStageNameBuffer[32];
			strcpy(parentRenderStageNameBuffer, "rs");
			strcpy(childRenderStageNameBuffer, "rs");

			uint32 internalRenderStageIndex = 0;

			for (const InternalRenderStage* pParentInternalRenderStage : sortedInternalRenderStages)
			{
				sprintf(parentRenderStageNameBuffer + 2, "%u", internalRenderStageIndex++);

				for (const InternalRenderStage* pChildInternalRenderStage : pParentInternalRenderStage->ChildRenderStages)
				{
					uint32 childIndex = (uint32)std::distance(sortedInternalRenderStages.begin(), std::find(sortedInternalRenderStages.begin(), sortedInternalRenderStages.end(), pChildInternalRenderStage));

					sprintf(childRenderStageNameBuffer + 2, "%u", childIndex);

					char edge[256];

					strcpy(edge, "\t");
					strcat(edge, parentRenderStageNameBuffer);
					strcat(edge, " -> ");
					strcat(edge, childRenderStageNameBuffer);
					strcat(edge, "[style=dashed];\n");

					edgesBuffer.push_back(edge);
				}
			}
		}

		for (const std::string& edge : edgesBuffer)
		{
			fputs(edge.c_str(), pFile);
		}

	}

	void RenderGraphDescriptionParser::WriteGraphVizCompleteDeclarations(
		FILE* pFile, 
		bool declareExternalInputs,
		std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStageDesc>&													sortedRenderStages,
		std::vector<SynchronizationStageDesc>&											sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&													sortedPipelineStages)
	{
		UNREFERENCED_VARIABLE(sortedInternalRenderStages);
		UNREFERENCED_VARIABLE(sortedRenderStages);
		UNREFERENCED_VARIABLE(sortedSynchronizationStages);
		UNREFERENCED_VARIABLE(sortedPipelineStages);

		char fileOutputBuffer[256];
		char renderStageBuffer[32];
		char renderStageInputAttachmentBuffer[32];
		char renderStageTemporalInputAttachmentBuffer[32];
		char renderStageExternalInputAttachmentBuffer[32];
		char renderStageOutputAttachmentBuffer[32];
		strcpy(renderStageBuffer, "rs");
		strcpy(renderStageInputAttachmentBuffer, "ia");
		strcpy(renderStageTemporalInputAttachmentBuffer, "tia");
		strcpy(renderStageExternalInputAttachmentBuffer, "eia");
		strcpy(renderStageOutputAttachmentBuffer, "oa");


		//Temporal Input Attachments
		fputs("\tsubgraph cluster0 {\n", pFile);
		fputs("\t\tnode [style=filled,color=white];\n", pFile);
		fputs("\t\tstyle = filled;\n", pFile);
		fputs("\t\tcolor = lightgrey;\n", pFile);
		fputs("\t\tlabel = \"Temporal Inputs\";\n", pFile);

		for (auto& attachmentPair : parsedTemporalInputAttachments)
		{
			const InternalRenderStageAttachment& renderStageTemporalInputAttachment = attachmentPair.second;

			sprintf(renderStageTemporalInputAttachmentBuffer + 3, "%u", renderStageTemporalInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageTemporalInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Temporal Input\\n");
			strcat(fileOutputBuffer, renderStageTemporalInputAttachment.AttachmentName.c_str());
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		fputs("\t}\n", pFile);

		if (declareExternalInputs)
		{
			//External Input Attachments
			fputs("\tsubgraph cluster1 {\n", pFile);
			fputs("\t\tnode [style=filled,color=white];\n", pFile);
			fputs("\t\tstyle = filled;\n", pFile);
			fputs("\t\tcolor = lightgrey;\n", pFile);
			fputs("\t\tlabel = \"External Inputs\";\n", pFile);

			for (auto& attachmentPair : parsedExternalInputAttachments)
			{
				const InternalRenderStageAttachment& renderStageExternalInputAttachment = attachmentPair.second;

				sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);

				strcpy(fileOutputBuffer, "\t\t");
				strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);
				strcat(fileOutputBuffer, " [shape=box,label=\"External Input\\n");
				strcat(fileOutputBuffer, renderStageExternalInputAttachment.AttachmentName.c_str());
				strcat(fileOutputBuffer, "\"];\n");
				fputs(fileOutputBuffer, pFile);
			}

			fputs("\t}\n", pFile);
		}

		fputs("\tsubgraph cluster2 {\n", pFile);
		fputs("\t\tstyle = filled;\n", pFile);
		fputs("\t\tcolor = gray90;\n", pFile);
		fputs("\t\tlabel = \"Main Pipeline\";\n", pFile);

		//Render Stages
		for (auto& renderStagePair : parsedRenderStages)
		{
			const InternalRenderStage& renderStage = renderStagePair.second;

			sprintf(renderStageBuffer + 2, "%u", renderStage.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageBuffer);
			strcat(fileOutputBuffer, " [shape=octagon,style=bold,label=\"");
			strcat(fileOutputBuffer, renderStage.pRenderStage->pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Input Attachments
		for (auto& attachmentPair : parsedInputAttachments)
		{
			const InternalRenderStageAttachment& renderStageInputAttachment = attachmentPair.second;

			sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Input\\n");
			strcat(fileOutputBuffer, renderStageInputAttachment.AttachmentName.c_str());
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Output Attachments
		for (auto& attachmentPair : parsedOutputAttachments)
		{
			const InternalRenderStageAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Output\\n");
			strcat(fileOutputBuffer, renderStageOutputAttachment.AttachmentName.c_str());
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		fputs("\t}\n", pFile);
	}

	void RenderGraphDescriptionParser::WriteGraphVizCompleteDefinitions(
		FILE* pFile, 
		bool externalInputsDeclared, 
		bool linkExternalInputs,
		std::unordered_map<std::string, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedTemporalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedExternalInputAttachments,
		std::unordered_map<std::string, InternalRenderStageAttachment>&					parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStageDesc>&													sortedRenderStages,
		std::vector<SynchronizationStageDesc>&											sortedSynchronizationStages,
		std::vector<PipelineStageDesc>&													sortedPipelineStages)
	{
		UNREFERENCED_VARIABLE(parsedInputAttachments);
		UNREFERENCED_VARIABLE(parsedTemporalInputAttachments);
		UNREFERENCED_VARIABLE(sortedInternalRenderStages);
		UNREFERENCED_VARIABLE(sortedRenderStages);
		UNREFERENCED_VARIABLE(sortedSynchronizationStages);
		UNREFERENCED_VARIABLE(sortedPipelineStages);

		char fileOutputBuffer[256];
		char renderStageBuffer[32];
		char renderStageInputAttachmentBuffer[32];
		char renderStageTemporalInputAttachmentBuffer[32];
		char renderStageExternalInputAttachmentBuffer[32];
		char renderStageOutputAttachmentBuffer[32];
		strcpy(renderStageBuffer, "rs");
		strcpy(renderStageInputAttachmentBuffer, "ia");
		strcpy(renderStageTemporalInputAttachmentBuffer, "tia");
		strcpy(renderStageExternalInputAttachmentBuffer, "eia");
		strcpy(renderStageOutputAttachmentBuffer, "oa");

		for (auto& renderStagePair : parsedRenderStages)
		{
			const InternalRenderStage& renderStage = renderStagePair.second;

			sprintf(renderStageBuffer + 2, "%u", renderStage.GlobalIndex);

			if (renderStage.InputAttachments.size() > 0)
			{
				strcpy(fileOutputBuffer, "\t{");

				//Input Attachments
				for (InternalRenderStageAttachment* pInputAttachment : renderStage.InputAttachments)
				{
					sprintf(renderStageInputAttachmentBuffer + 2, "%u", pInputAttachment->GlobalIndex);

					strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);

					if (pInputAttachment != renderStage.InputAttachments.back())
						strcat(fileOutputBuffer, ", ");
				}

				strcat(fileOutputBuffer, "} -> ");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, "[color=green3];\n");

				fputs(fileOutputBuffer, pFile);
			}

			if (renderStage.TemporalInputAttachments.size() > 0)
			{
				strcpy(fileOutputBuffer, "\t{");

				//Temporal Input Attachments
				for (InternalRenderStageAttachment* pTemporalInputAttachment : renderStage.TemporalInputAttachments)
				{
					sprintf(renderStageTemporalInputAttachmentBuffer + 3, "%u", pTemporalInputAttachment->GlobalIndex);

					strcat(fileOutputBuffer, renderStageTemporalInputAttachmentBuffer);

					if (pTemporalInputAttachment != renderStage.TemporalInputAttachments.back())
						strcat(fileOutputBuffer, ", ");
				}

				strcat(fileOutputBuffer, "} -> ");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, "[color=blue];\n");

				fputs(fileOutputBuffer, pFile);
			}

			if (externalInputsDeclared && linkExternalInputs)
			{
				if (renderStage.ExternalInputAttachments.size() > 0)
				{
					strcpy(fileOutputBuffer, "\t{");

					//External Input Attachments
					for (InternalRenderStageAttachment* pExternalInputAttachment : renderStage.ExternalInputAttachments)
					{
						sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", pExternalInputAttachment->GlobalIndex);

						strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);

						if (pExternalInputAttachment != renderStage.ExternalInputAttachments.back())
							strcat(fileOutputBuffer, ", ");
					}

					strcat(fileOutputBuffer, "} -> ");
					strcat(fileOutputBuffer, renderStageBuffer);
					strcat(fileOutputBuffer, "[style=dashed];\n");

					fputs(fileOutputBuffer, pFile);
				}
			}
		}

		if (externalInputsDeclared && !linkExternalInputs)
		{
			strcpy(fileOutputBuffer, "\t");

			for (auto attachmentPairIt = parsedExternalInputAttachments.begin(); attachmentPairIt != parsedExternalInputAttachments.end();)
			{
				const InternalRenderStageAttachment& renderStageExternalInputAttachment = attachmentPairIt->second;

				sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);
				
				strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);

				attachmentPairIt++;

				if (attachmentPairIt != parsedExternalInputAttachments.end())
					strcat(fileOutputBuffer, " -> ");
			}

			strcat(fileOutputBuffer, "[style=invis];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Output Attachments
		for (auto& attachmentPair : parsedOutputAttachments)
		{
			const InternalRenderStageAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			for (auto renderStageIt = renderStageOutputAttachment.RenderStageNameToRenderStageAndAttachment.begin(); renderStageIt != renderStageOutputAttachment.RenderStageNameToRenderStageAndAttachment.end(); renderStageIt++)
			{
				sprintf(renderStageBuffer + 2, "%u", renderStageIt->second.first->GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
				strcat(fileOutputBuffer, "[color=purple];\n");
				fputs(fileOutputBuffer, pFile);
			}

			if (renderStageOutputAttachment.pConnectedAttachment != nullptr)
			{
				sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.pConnectedAttachment->GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
				strcat(fileOutputBuffer, "[style=bold,color=red];\n");
				fputs(fileOutputBuffer, pFile);
			}
		}
	}

	void RenderGraphDescriptionParser::SanitizeString(char* pString, uint32 numCharacters)
	{
		static std::string illegalChars = "\\/:?\"<>| ";
		static std::string capitalLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

		for (uint32_t i = 0; i < numCharacters; i++)
		{
			if (illegalChars.find(pString[i]) != std::string::npos)
			{
				pString[i] = '_';
			}
			else if (capitalLetters.find(pString[i]) != std::string::npos)
			{
				pString[i] += 32;
			}
		}
	}

	void RenderGraphDescriptionParser::ConcatPipelineStateToString(char* pStringBuffer, EPipelineStateType pipelineState)
	{
		switch (pipelineState)
		{
		case EPipelineStateType::GRAPHICS:		strcat(pStringBuffer, "GRAPHICS");	break;
		case EPipelineStateType::COMPUTE:		strcat(pStringBuffer, "COMPUTE");	break;
		case EPipelineStateType::RAY_TRACING:	strcat(pStringBuffer, "COMPUTE");	break;
		}
	}
}