#include "Rendering/RenderGraphCreator.h"

#include "Log/Log.h"

#include <cstdio>
#include <map>
#include <unordered_set>

namespace LambdaEngine
{
	bool RenderGraphCreator::Create(const RenderGraphDesc& desc)
	{
		std::unordered_map<const char*, InternalRenderStage>						parsedRenderStages;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			parsedInputAttachments;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			parsedTemporalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>	parsedExternalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>		parsedOutputAttachments;

		std::vector<const InternalRenderStage*>										sortedInternalRenderStages;

		std::vector<RenderStage>													sortedRenderStages;
		std::vector<SynchronizationStage>											sortedSynchronizationStages;
		std::vector<PipelineStage>													sortedPipelineStages;
		
		if (!ParseInitialStages(
			desc,
			parsedRenderStages,
			parsedInputAttachments,
			parsedTemporalInputAttachments,
			parsedExternalInputAttachments,
			parsedOutputAttachments))
		{
			LOG_ERROR("[RenderGraph]: Could not parse Render Stages for \"%s\"", desc.pName);
			return false;
		}

		if (!ConnectOutputsToInputs(
			parsedInputAttachments,
			parsedOutputAttachments))
		{
			LOG_ERROR("[RenderGraph]: Could not connect Output Resources to Input Resources for \"%s\"", desc.pName);
			return false;
		}

		if (!WeightRenderStages(
			parsedRenderStages))
		{
			LOG_ERROR("[RenderGraph]: Could not weights render stages for \"%s\"", desc.pName);
			return false;
		}

		if (!SortPipelineStages(
			parsedRenderStages,
			sortedInternalRenderStages,
			sortedRenderStages,
			sortedSynchronizationStages,
			sortedPipelineStages))
		{
			LOG_ERROR("[RenderGraph]: Could not sort render stages for \"%s\"", desc.pName);
			return false;
		}

		if (!PruneUnnecessarySynchronizations(
			sortedSynchronizationStages,
			sortedPipelineStages))
		{
			LOG_ERROR("[RenderGraph]: Could not prunt unnecessary synchronization for \"%s\"", desc.pName);
			return false;
		}

		if (desc.CreateDebugGraph)
		{
			constexpr bool DECLARE_EXTERNAL_INPUTS = true;
			constexpr bool LINK_EXTERNAL_INPUTS = false;

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
				sortedRenderStages,
				sortedSynchronizationStages,
				sortedPipelineStages))
			{
				LOG_WARNING("[RenderGraph]: Could not create GraphViz for \"%s\"", desc.pName);
			}
		}
		
		return true;
	}

	bool RenderGraphCreator::ParseInitialStages(
		const RenderGraphDesc& desc,
		std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments)
	{
		uint32 globalAttachmentIndex = 0;

		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStage& renderStage = desc.pRenderStages[renderStageIndex];

			auto renderStageIt = parsedRenderStages.find(renderStage.pName);

			if (renderStageIt != parsedRenderStages.end())
			{
				LOG_ERROR("[RenderGraph]: Multiple Render Stages with same name is not allowed, name: \"%s\"", renderStage.pName);
				return false;
			}

			InternalRenderStage& internalRenderStage = parsedRenderStages[renderStage.pName];
			internalRenderStage.pRenderStage = &renderStage;
			internalRenderStage.GlobalIndex = renderStageIndex;

			//Input Attachments
			for (uint32 inputAttachmentIndex = 0; inputAttachmentIndex < renderStage.InputAttachmentCount; inputAttachmentIndex++)
			{
				const RenderStageInputAttachment& renderStageInputAttachment = renderStage.pInputAttachments[inputAttachmentIndex];
				auto inputIt = parsedInputAttachments.find(renderStageInputAttachment.pName);
				auto temporalInputIt = parsedTemporalInputAttachments.find(renderStageInputAttachment.pName);

				bool isTemporal = IsInputTemporal(renderStage, &renderStageInputAttachment);

				if (isTemporal)
				{
					//Temporal
					if (temporalInputIt != parsedTemporalInputAttachments.end() &&
						temporalInputIt->second.pAttachment->Type == renderStageInputAttachment.Type)
					{
						temporalInputIt->second.RenderStages.push_back(&internalRenderStage);
						internalRenderStage.TemporalInputAttachments.push_back(&temporalInputIt->second);
					}
					else
					{
						InternalRenderStageInputAttachment& internalTemporalInputAttachment = parsedTemporalInputAttachments[renderStageInputAttachment.pName];
						internalTemporalInputAttachment.pAttachment = &renderStageInputAttachment;
						internalTemporalInputAttachment.RenderStages.push_back(&internalRenderStage);
						internalTemporalInputAttachment.GlobalIndex = globalAttachmentIndex;
						globalAttachmentIndex++;

						internalRenderStage.TemporalInputAttachments.push_back(&internalTemporalInputAttachment);
					}
				}
				else
				{
					//Non-Temporal
					if (inputIt != parsedInputAttachments.end() &&
						inputIt->second.pAttachment->Type == renderStageInputAttachment.Type)
					{
						inputIt->second.RenderStages.push_back(&internalRenderStage);
						internalRenderStage.InputAttachments.push_back(&inputIt->second);
					}
					else
					{
						InternalRenderStageInputAttachment& internalInputAttachment = parsedInputAttachments[renderStageInputAttachment.pName];
						internalInputAttachment.pAttachment = &renderStageInputAttachment;
						internalInputAttachment.RenderStages.push_back(&internalRenderStage);
						internalInputAttachment.GlobalIndex = globalAttachmentIndex;
						globalAttachmentIndex++;

						internalRenderStage.InputAttachments.push_back(&internalInputAttachment);
					}
				}
			}

			//External Input Attachments
			for (uint32 externalInputAttachmentIndex = 0; externalInputAttachmentIndex < renderStage.ExtenalInputAttachmentCount; externalInputAttachmentIndex++)
			{
				const RenderStageExternalInputAttachment& renderStageExternalInputAttachment = renderStage.pExternalInputAttachments[externalInputAttachmentIndex];
				auto externalInputIt = parsedExternalInputAttachments.find(renderStageExternalInputAttachment.pName);

				if (externalInputIt != parsedExternalInputAttachments.end() &&
					externalInputIt->second.pAttachment->Type == renderStageExternalInputAttachment.Type &&
					externalInputIt->second.pAttachment->DescriptorCount == renderStageExternalInputAttachment.DescriptorCount)
				{
					externalInputIt->second.RenderStages.push_back(&internalRenderStage);
					internalRenderStage.ExternalInputAttachments.push_back(&externalInputIt->second);
				}
				else
				{
					InternalRenderStageExternalInputAttachment& internalExternalInputAttachment = parsedExternalInputAttachments[renderStageExternalInputAttachment.pName];
					internalExternalInputAttachment.pAttachment = &renderStageExternalInputAttachment;
					internalExternalInputAttachment.RenderStages.push_back(&internalRenderStage);
					internalExternalInputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalRenderStage.ExternalInputAttachments.push_back(&internalExternalInputAttachment);
				}
			}

			//Output Attachments
			for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStage.OutputAttachmentCount; outputAttachmentIndex++)
			{
				const RenderStageOutputAttachment& renderStageOutputAttachment = renderStage.pOutputAttachments[outputAttachmentIndex];
				auto outputIt = parsedOutputAttachments.find(renderStageOutputAttachment.pName);

				if (outputIt != parsedOutputAttachments.end() &&
					outputIt->second.pAttachment->Type == renderStageOutputAttachment.Type)
				{
					outputIt->second.RenderStages.push_back(&internalRenderStage);
					internalRenderStage.OutputAttachments.push_back(&outputIt->second);
				}
				else
				{
					InternalRenderStageOutputAttachment& internalOutputAttachment = parsedOutputAttachments[renderStageOutputAttachment.pName];
					internalOutputAttachment.pAttachment = &renderStageOutputAttachment;
					internalOutputAttachment.RenderStages.push_back(&internalRenderStage);
					internalOutputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalRenderStage.OutputAttachments.push_back(&internalOutputAttachment);
				}
			}
		}

		return true;
	}

	bool RenderGraphCreator::ConnectOutputsToInputs(
		std::unordered_map<const char*, InternalRenderStageInputAttachment>& parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>& parsedOutputAttachments)
	{
		//Connect Output Attachments to Input Attachments
		for (auto& outputAttachmentPair : parsedOutputAttachments)
		{
			InternalRenderStageOutputAttachment& renderStageOutputAttachment = outputAttachmentPair.second;

			for (auto& inputAttachmentPair : parsedInputAttachments)
			{
				InternalRenderStageInputAttachment& renderStageInputAttachment = inputAttachmentPair.second;

				if (AttachmentsEqual(renderStageInputAttachment.pAttachment, renderStageOutputAttachment.pAttachment))
				{
					//Connect Render Stages
					for (InternalRenderStage* pPreviousRenderStages : renderStageOutputAttachment.RenderStages)
					{
						for (InternalRenderStage* pNextRenderStages : renderStageInputAttachment.RenderStages)
						{
							if (AreRenderStagesRelated(pNextRenderStages, pPreviousRenderStages))
							{
								LOG_ERROR("[RenderGraph]: Cyclic Render Stage dependency detected between \"%s\" and \"%s\"!", pPreviousRenderStages->pRenderStage->pName, pNextRenderStages->pRenderStage->pName);
								return false;
							}

							pPreviousRenderStages->ChildRenderStages.insert(pNextRenderStages);
							pNextRenderStages->ParentRenderStages.insert(pPreviousRenderStages);
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
			InternalRenderStageInputAttachment& renderStageInputAttachment = inputAttachmentPair.second;

			if (renderStageInputAttachment.pConnectedAttachment == nullptr)
			{
				result = false;
				LOG_ERROR("[RenderGraph]: Input Attachment \"%s\" has not connected output!", renderStageInputAttachment.pAttachment->pName);
			}
		}

		return result;
	}

	bool RenderGraphCreator::WeightRenderStages(std::unordered_map<const char*, InternalRenderStage>& parsedRenderStages)
	{
		for (auto renderStageIt = parsedRenderStages.begin(); renderStageIt != parsedRenderStages.end(); renderStageIt++)
		{
			RecursivelyWeightAncestors(&renderStageIt->second);
		}

		return true;
	}

	void RenderGraphCreator::RecursivelyWeightAncestors(InternalRenderStage* pRenderStage)
	{
		for (InternalRenderStage* pParentRenderStage : pRenderStage->ParentRenderStages)
		{
			pParentRenderStage->Weight++;
			RecursivelyWeightAncestors(pParentRenderStage);
		}
	}

	bool RenderGraphCreator::SortPipelineStages(
		std::unordered_map<const char*, InternalRenderStage>&	parsedRenderStages,
		std::vector<const InternalRenderStage*>&				sortedInternalRenderStages,
		std::vector<RenderStage>&								sortedRenderStages,
		std::vector<SynchronizationStage>&						sortedSynchronizationStages,
		std::vector<PipelineStage>&								sortedPipelineStages)
	{
		std::multimap<uint32, const InternalRenderStage*> weightedRenderStageMap;

		for (auto renderStageIt = parsedRenderStages.begin(); renderStageIt != parsedRenderStages.end(); renderStageIt++)
		{
			weightedRenderStageMap.insert(std::make_pair(renderStageIt->second.Weight, &renderStageIt->second));
		}

		std::unordered_map<const char*, std::pair<const RenderStageInputAttachment*, EPipelineStateType>> finalStateOfAttachments;

		for (auto sortedRenderStageIt = weightedRenderStageMap.begin(); sortedRenderStageIt != weightedRenderStageMap.end(); sortedRenderStageIt++)
		{
			sortedInternalRenderStages.push_back(sortedRenderStageIt->second);

			const RenderStage* pSourceRenderStage = sortedRenderStageIt->second->pRenderStage;

			RenderStage renderStage = {};
			renderStage.pName							= pSourceRenderStage->pName;
			renderStage.pInputAttachments				= pSourceRenderStage->pInputAttachments;
			renderStage.pExternalInputAttachments		= pSourceRenderStage->pExternalInputAttachments;
			renderStage.pOutputAttachments				= pSourceRenderStage->pOutputAttachments;
			renderStage.InputAttachmentCount			= pSourceRenderStage->InputAttachmentCount;
			renderStage.ExtenalInputAttachmentCount		= pSourceRenderStage->ExtenalInputAttachmentCount;
			renderStage.OutputAttachmentCount			= pSourceRenderStage->OutputAttachmentCount;
			
			renderStage.PipelineType					= pSourceRenderStage->PipelineType;
			
			switch (renderStage.PipelineType)
			{
			case EPipelineStateType::GRAPHICS:		renderStage.Pipeline.pGraphicsDesc = pSourceRenderStage->Pipeline.pGraphicsDesc; break;
			case EPipelineStateType::COMPUTE:		renderStage.Pipeline.pComputeDesc = pSourceRenderStage->Pipeline.pComputeDesc; break;
			case EPipelineStateType::RAY_TRACING:	renderStage.Pipeline.pRayTracingDesc = pSourceRenderStage->Pipeline.pRayTracingDesc; break;
			}		

			sortedRenderStages.push_back(renderStage);

			PipelineStage pipelineStage = {};
			pipelineStage.Type = EPipelineStageType::RENDER;
			pipelineStage.StageIndex = sortedRenderStages.size() - 1;
			sortedPipelineStages.push_back(pipelineStage);

			SynchronizationStage synchronizationStage = {};

			//Create Output to Input Synchronizations
			{
				for (const InternalRenderStageInputAttachment* pInputAttachment : sortedRenderStageIt->second->InputAttachments)
				{
					AttachmentSynchronization attachmentSynchronization = {};
					attachmentSynchronization.Type							= EAttachmentSynchronizationType::TRANSITION_FOR_READ;
					attachmentSynchronization.ToQueueOwner					= sortedRenderStageIt->second->pRenderStage->PipelineType;
					attachmentSynchronization.OutputToInput.FromAttachment	= *pInputAttachment->pConnectedAttachment->pAttachment;
					attachmentSynchronization.OutputToInput.ToAttachment	= *pInputAttachment->pAttachment;

					synchronizationStage.Synchronizations.push_back(attachmentSynchronization);

					auto finalStateOfAttachmentIt = finalStateOfAttachments.find(pInputAttachment->pAttachment->pName);

					if (finalStateOfAttachmentIt == finalStateOfAttachments.end())
					{
						finalStateOfAttachments[pInputAttachment->pAttachment->pName] = std::make_pair(pInputAttachment->pAttachment, sortedRenderStageIt->second->pRenderStage->PipelineType);
					}
				}
			}

			//Create Input (From Prev Frame) to Output Synchronizations
			{
				for (const InternalRenderStageOutputAttachment* pOutputAttachment : sortedRenderStageIt->second->OutputAttachments)
				{
					if (!IsAttachmentReserved(pOutputAttachment->pAttachment->pName))
					{
						auto finalStateOfAttachmentIt = finalStateOfAttachments.find(pOutputAttachment->pAttachment->pName);

						if (finalStateOfAttachmentIt != finalStateOfAttachments.end())
						{
							AttachmentSynchronization attachmentSynchronization = {};
							attachmentSynchronization.Type							= EAttachmentSynchronizationType::TRANSITION_FOR_WRITE;
							attachmentSynchronization.FromQueueOwner				= finalStateOfAttachmentIt->second.second;
							attachmentSynchronization.ToQueueOwner					= sortedRenderStageIt->second->pRenderStage->PipelineType;
							attachmentSynchronization.InputToOutput.FromAttachment	= *finalStateOfAttachmentIt->second.first;
							attachmentSynchronization.InputToOutput.ToAttachment	= *pOutputAttachment->pAttachment;

							synchronizationStage.Synchronizations.push_back(attachmentSynchronization);
						}
					}
				}
			}

			//Push Synchronization Stage to Graph
			if (synchronizationStage.Synchronizations.size() > 0)
			{
				sortedSynchronizationStages.push_back(synchronizationStage);

				PipelineStage pipelineStage = {};
				pipelineStage.Type = EPipelineStageType::SYNCHRONIZATION;
				pipelineStage.StageIndex = sortedSynchronizationStages.size() - 1;
				sortedPipelineStages.push_back(pipelineStage);
			}
		}

		std::reverse(sortedRenderStages.begin(), sortedRenderStages.end());
		std::reverse(sortedSynchronizationStages.begin(), sortedSynchronizationStages.end());

		std::reverse(sortedInternalRenderStages.begin(), sortedInternalRenderStages.end());
		std::reverse(sortedPipelineStages.begin(), sortedPipelineStages.end());

		for (auto sortedPipelineStagesIt = sortedPipelineStages.begin(); sortedPipelineStagesIt != sortedPipelineStages.end(); sortedPipelineStagesIt++)
		{
			if (sortedPipelineStagesIt->Type == EPipelineStageType::RENDER)
			{
				sortedPipelineStagesIt->StageIndex = sortedRenderStages.size() - sortedPipelineStagesIt->StageIndex - 1;
			}
			else if (sortedPipelineStagesIt->Type == EPipelineStageType::SYNCHRONIZATION)
			{
				sortedPipelineStagesIt->StageIndex = sortedSynchronizationStages.size() - sortedPipelineStagesIt->StageIndex - 1;
			}
		}

		return true;
	}

	bool RenderGraphCreator::PruneUnnecessarySynchronizations(
		std::vector<SynchronizationStage>&	sortedSynchronizationStages,
		std::vector<PipelineStage>&			sortedPipelineStages)
	{
		std::unordered_map<const char*, AttachmentSynchronization*> firstEncounterOfAttachmentSynchronizations;
		std::unordered_map<const char*, std::pair<EAttachmentState, EPipelineStateType>> transitionedResourceStates;

		for (auto synchronizationStageIt = sortedSynchronizationStages.begin(); synchronizationStageIt != sortedSynchronizationStages.end();)
		{
			for (auto attachmentSynchronizationIt = synchronizationStageIt->Synchronizations.begin(); attachmentSynchronizationIt != synchronizationStageIt->Synchronizations.end(); )
			{
				const char* pAttachmentName = attachmentSynchronizationIt->OutputToInput.FromAttachment.pName;
				auto firstEncounterOfAttachmentSynchronizationIt = firstEncounterOfAttachmentSynchronizations.find(pAttachmentName);

				if (firstEncounterOfAttachmentSynchronizationIt == firstEncounterOfAttachmentSynchronizations.end())
				{
					firstEncounterOfAttachmentSynchronizations[pAttachmentName] = &(*attachmentSynchronizationIt);
				}

				auto transitionedResourceStateIt = transitionedResourceStates.find(pAttachmentName);

				if (transitionedResourceStateIt != transitionedResourceStates.end())
				{
					//Only TRANSITION_FOR_READ needs repairing
					if (transitionedResourceStateIt->second.first == EAttachmentState::READ)
					{
						if (attachmentSynchronizationIt->ToQueueOwner != transitionedResourceStateIt->second.second)
						{
							attachmentSynchronizationIt->FromQueueOwner = transitionedResourceStateIt->second.second;
							attachmentSynchronizationIt->Type = EAttachmentSynchronizationType::OWNERSHIP_CHANGE;
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
				case EAttachmentSynchronizationType::OWNERSHIP_CHANGE:
					resultState = transitionedResourceStateIt != transitionedResourceStates.end() ? transitionedResourceStateIt->second.first : EAttachmentState::NONE;
					break;
				case EAttachmentSynchronizationType::TRANSITION_FOR_READ:
					resultState = EAttachmentState::READ;
					break;
				case EAttachmentSynchronizationType::TRANSITION_FOR_WRITE:
					resultState = EAttachmentState::WRITE;
					break;
				}

				transitionedResourceStates[pAttachmentName] = std::make_pair(resultState, attachmentSynchronizationIt->ToQueueOwner);
				attachmentSynchronizationIt++;
			}

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

		for (auto firstEncounterSynchronizationIt = firstEncounterOfAttachmentSynchronizations.begin(); firstEncounterSynchronizationIt != firstEncounterOfAttachmentSynchronizations.end(); firstEncounterSynchronizationIt++)
		{
			firstEncounterSynchronizationIt->second->FromQueueOwner = transitionedResourceStates[firstEncounterSynchronizationIt->first].second;
		}

		return true;
	}

	bool RenderGraphCreator::IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment* pInputAttachment)
	{
		for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStage.OutputAttachmentCount; outputAttachmentIndex++)
		{
			const RenderStageOutputAttachment& renderStageOutputAttachment = renderStage.pOutputAttachments[outputAttachmentIndex];

			if (AttachmentsEqual(pInputAttachment, &renderStageOutputAttachment))
			{
				return true;
			}
		}

		return false;
	}

	bool RenderGraphCreator::AttachmentsEqual(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment)
	{
		return (strcmp(pInputAttachment->pName, pOutputAttachment->pName) == 0);
	}

	bool RenderGraphCreator::AreRenderStagesRelated(const InternalRenderStage* pRenderStageAncestor, const InternalRenderStage* pRenderStageDescendant)
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

	bool RenderGraphCreator::IsAttachmentReserved(const char* pAttachmentName)
	{
		return strcmp(pAttachmentName, RENDER_GRAPH_BACK_BUFFER) == 0;
	}

	bool RenderGraphCreator::WriteGraphViz(
		const char* pName, 
		bool declareExternalInputs, 
		bool linkExternalInputs,
		std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStage>&														sortedRenderStages,
		std::vector<SynchronizationStage>&												sortedSynchronizationStages,
		std::vector<PipelineStage>&														sortedPipelineStages)
	{
		char renderGraphNameBuffer[256];
		char filepathBuffer[256];

		{
			strcpy(renderGraphNameBuffer, pName);

			SanitizeString(renderGraphNameBuffer, strlen(renderGraphNameBuffer));

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

			SanitizeString(renderGraphNameBuffer, strlen(renderGraphNameBuffer));

			strcpy(filepathBuffer, "complete_");
			strcat(filepathBuffer, renderGraphNameBuffer);
			strcat(filepathBuffer, ".dot");

			SanitizeString(filepathBuffer, strlen(filepathBuffer));

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

	void RenderGraphCreator::WriteGraphVizPipelineStages(
		FILE* pFile,
		std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStage>&														sortedRenderStages,
		std::vector<SynchronizationStage>&												sortedSynchronizationStages,
		std::vector<PipelineStage>&														sortedPipelineStages)
	{
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

			for (const PipelineStage& pipelineStage : sortedPipelineStages)
			{
				if (pipelineStage.Type == EPipelineStageType::RENDER)
				{
					const RenderStage& renderStage = sortedRenderStages[pipelineStage.StageIndex];

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
					const SynchronizationStage& synchronizationStage = sortedSynchronizationStages[pipelineStage.StageIndex];

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

					for (const AttachmentSynchronization& synchronization : synchronizationStage.Synchronizations)
					{
						sprintf(attachmentSynchronizationNameBuffer + 2, "%u", attachmentSynchronizationIndex++);

						strcpy(fileOutputBuffer, "\t\t");
						strcat(fileOutputBuffer, attachmentSynchronizationNameBuffer);
						strcat(fileOutputBuffer, " [shape=box,label=\"");

						if (synchronization.Type == EAttachmentSynchronizationType::TRANSITION_FOR_READ)
						{
							strcat(fileOutputBuffer, synchronization.OutputToInput.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--TRANSITION FOR READ--\\n");
						}
						else if (synchronization.Type == EAttachmentSynchronizationType::OWNERSHIP_CHANGE)
						{
							strcat(fileOutputBuffer, synchronization.OutputToInput.FromAttachment.pName);
							strcat(fileOutputBuffer, "\\n--OWNERSHIP CHANGE--\\n");
						}
						else if (synchronization.Type == EAttachmentSynchronizationType::TRANSITION_FOR_WRITE)
						{
							strcat(fileOutputBuffer, synchronization.InputToOutput.FromAttachment.pName);
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
					uint32 childIndex = std::distance(sortedInternalRenderStages.begin(), std::find(sortedInternalRenderStages.begin(), sortedInternalRenderStages.end(), pChildInternalRenderStage));

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

	void RenderGraphCreator::WriteGraphVizCompleteDeclarations(
		FILE* pFile, 
		bool declareExternalInputs,
		std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStage>&														sortedRenderStages,
		std::vector<SynchronizationStage>&												sortedSynchronizationStages,
		std::vector<PipelineStage>&														sortedPipelineStages)
	{
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
			const InternalRenderStageInputAttachment& renderStageTemporalInputAttachment = attachmentPair.second;

			sprintf(renderStageTemporalInputAttachmentBuffer + 3, "%u", renderStageTemporalInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageTemporalInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Temporal Input\\n");
			strcat(fileOutputBuffer, renderStageTemporalInputAttachment.pAttachment->pName);
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
				const InternalRenderStageExternalInputAttachment& renderStageExternalInputAttachment = attachmentPair.second;

				sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);

				strcpy(fileOutputBuffer, "\t\t");
				strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);
				strcat(fileOutputBuffer, " [shape=box,label=\"External Input\\n");
				strcat(fileOutputBuffer, renderStageExternalInputAttachment.pAttachment->pName);
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
			const InternalRenderStageInputAttachment& renderStageInputAttachment = attachmentPair.second;

			sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Input\\n");
			strcat(fileOutputBuffer, renderStageInputAttachment.pAttachment->pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Output Attachments
		for (auto& attachmentPair : parsedOutputAttachments)
		{
			const InternalRenderStageOutputAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"Output\\n");
			strcat(fileOutputBuffer, renderStageOutputAttachment.pAttachment->pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		fputs("\t}\n", pFile);
	}

	void RenderGraphCreator::WriteGraphVizCompleteDefinitions(
		FILE* pFile, 
		bool externalInputsDeclared, 
		bool linkExternalInputs,
		std::unordered_map<const char*, InternalRenderStage>&							parsedRenderStages,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedInputAttachments,
		std::unordered_map<const char*, InternalRenderStageInputAttachment>&			parsedTemporalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>&	parsedExternalInputAttachments,
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>&			parsedOutputAttachments,
		std::vector<const InternalRenderStage*>&										sortedInternalRenderStages,
		std::vector<RenderStage>&														sortedRenderStages,
		std::vector<SynchronizationStage>&												sortedSynchronizationStages,
		std::vector<PipelineStage>&														sortedPipelineStages)
	{
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

		uint32 invisibleNodeCounter = 0;

		for (auto& renderStagePair : parsedRenderStages)
		{
			const InternalRenderStage& renderStage = renderStagePair.second;

			sprintf(renderStageBuffer + 2, "%u", renderStage.GlobalIndex);

			if (renderStage.InputAttachments.size() > 0)
			{
				strcpy(fileOutputBuffer, "\t{");

				//Input Attachments
				for (InternalRenderStageInputAttachment* pInputAttachment : renderStage.InputAttachments)
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
				for (InternalRenderStageInputAttachment* pTemporalInputAttachment : renderStage.TemporalInputAttachments)
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
					for (InternalRenderStageExternalInputAttachment* pExternalInputAttachment : renderStage.ExternalInputAttachments)
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
				const InternalRenderStageExternalInputAttachment& renderStageExternalInputAttachment = attachmentPairIt->second;

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
			const InternalRenderStageOutputAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			for (InternalRenderStage* pRenderStage : renderStageOutputAttachment.RenderStages)
			{
				sprintf(renderStageBuffer + 2, "%u", pRenderStage->GlobalIndex);

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

	void RenderGraphCreator::SanitizeString(char* pString, uint32 numCharacters)
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

	void RenderGraphCreator::ConcatPipelineStateToString(char* pStringBuffer, EPipelineStateType pipelineState)
	{
		switch (pipelineState)
		{
		case EPipelineStateType::GRAPHICS:		strcat(pStringBuffer, "GRAPHICS");	break;
		case EPipelineStateType::COMPUTE:		strcat(pStringBuffer, "COMPUTE");	break;
		case EPipelineStateType::RAY_TRACING:	strcat(pStringBuffer, "COMPUTE");	break;
		}
	}
}