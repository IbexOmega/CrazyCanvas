#include "PreCompiled.h"
#include "Rendering/RenderGraphParser.h"

namespace LambdaEngine
{
	bool RenderGraphParser::ParseRenderGraph(
		RenderGraphStructureDesc* pParsedStructure, 
		const TArray<RenderGraphResourceDesc>& resources, 
		const THashTable<String, EditorRenderStageDesc>& renderStagesByName,
		const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex, 
		const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex, 
		const EditorFinalOutput& finalOutput, 
		bool generateImGuiStage)
	{
		if (!generateImGuiStage)
		{
			auto backBufferFinalStateIt = resourceStatesByHalfAttributeIndex.find(finalOutput.BackBufferAttributeIndex / 2);

			if (backBufferFinalStateIt == resourceStatesByHalfAttributeIndex.end())
			{
				LOG_ERROR("[RenderGraphParser]: Back Buffer does not exist in Resource Map");
				return false;
			}

			if (backBufferFinalStateIt->second.InputLinkIndex == -1)
			{
				LOG_ERROR("[RenderGraphParser]: No link connected to Final Output");
				return false;
			}

			auto backBufferFinalLink = resourceStateLinksByLinkIndex.find(backBufferFinalStateIt->second.InputLinkIndex);

			if (backBufferFinalLink == resourceStateLinksByLinkIndex.end())
			{
				LOG_ERROR("[RenderGraphParser]: Link to Final Output was not found in Link Map");
				return false;
			}

			//Get the Render Stage connected to the Final Output Stage
			auto backBufferFinalResourceState = resourceStatesByHalfAttributeIndex.find(backBufferFinalLink->second.SrcAttributeIndex / 2);

			if (backBufferFinalResourceState == resourceStatesByHalfAttributeIndex.end())
			{
				LOG_ERROR("[RenderGraphParser]: Final Resource State for Back Buffer was not found in Resource State Map");
				return false;
			}

			//Check if the final Render Stage actually is a Render Stage and not a Resource State Group
			if (renderStagesByName.count(backBufferFinalResourceState->second.RenderStageName) == 0)
			{
				LOG_ERROR("[RenderGraphParser]: A valid render stage must be linked to %s", finalOutput.Name.c_str());
				return false;
			}
		}

		//Create Render Stage Weight Map
		THashTable<String, int32> renderStageWeightsByName;

		for (auto renderStageIt = renderStagesByName.begin(); renderStageIt != renderStagesByName.end(); renderStageIt++)
		{
			renderStageWeightsByName[renderStageIt->first] = 0;
		}

		//Weight Render Stages
		for (auto renderStageIt = renderStagesByName.begin(); renderStageIt != renderStagesByName.end(); renderStageIt++)
		{
			if (!RecursivelyWeightParentRenderStages(
				renderStageIt,
				renderStagesByName,
				resourceStatesByHalfAttributeIndex,
				resourceStateLinksByLinkIndex,
				renderStageWeightsByName,
				UINT32_MAX))
			{
				LOG_ERROR("[RenderGraphParser]: Failed to recursively weight Render Stages");
				return false;
			}
		}

		//Created sorted Map of Render Stages
		std::multimap<uint32, const EditorRenderStageDesc*> orderedMappedRenderStages;
		for (auto renderStageIt = renderStagesByName.begin(); renderStageIt != renderStagesByName.end(); renderStageIt++)
		{
			auto renderStageWeightIt = renderStageWeightsByName.find(renderStageIt->first);

			if (renderStageWeightIt != renderStageWeightsByName.end())
			{
				orderedMappedRenderStages.insert({ renderStageWeightIt->second, &renderStageIt->second });
			}
			else
			{
				LOG_ERROR("[RenderGraphParser]: Failed to create ordered Render Stages, this should not happen");
				return false;
			}
		}

		TArray<RenderStageDesc>				orderedRenderStages;
		TArray<SynchronizationStageDesc>	orderedSynchronizationStages;
		TArray<PipelineStageDesc>			orderedPipelineStages;

		orderedRenderStages.Reserve(uint32(orderedMappedRenderStages.size()));
		orderedSynchronizationStages.Reserve(uint32(orderedMappedRenderStages.size()));
		orderedPipelineStages.Reserve(uint32(2 * orderedMappedRenderStages.size()));

		THashTable<String, const EditorRenderGraphResourceState*> finalStateOfResources;

		RenderStageDesc imguiRenderStage = {};

		if (generateImGuiStage)
		{
			imguiRenderStage.Name				= RENDER_GRAPH_IMGUI_STAGE_NAME;
			imguiRenderStage.Type				= EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS;
			imguiRenderStage.CustomRenderer		= true;
			imguiRenderStage.TriggerType		= ERenderStageExecutionTrigger::EVERY;
			imguiRenderStage.FrameDelay			= 0;
			imguiRenderStage.FrameOffset		= 0;
		}

		//Loop Through each Render Stage in Order and create synchronization stages
		TSet<String> resourceNamesActuallyUsed;
		for (auto orderedRenderStageIt = orderedMappedRenderStages.rbegin(); orderedRenderStageIt != orderedMappedRenderStages.rend(); orderedRenderStageIt++)
		{
			const EditorRenderStageDesc* pCurrentRenderStage = orderedRenderStageIt->second;

			SynchronizationStageDesc synchronizationStage = {};

			//Loop through each Resource State in the Render Stage
			for (const EditorResourceStateIdent& resourceStateIdent : pCurrentRenderStage->ResourceStateIdents)
			{
				auto currentResourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceStateIdent.AttributeIndex / 2);

				if (currentResourceStateIt != resourceStatesByHalfAttributeIndex.end())
				{
					resourceNamesActuallyUsed.insert(resourceStateIdent.Name);
					if (FindAndCreateSynchronization(resources, resourceStatesByHalfAttributeIndex, orderedRenderStageIt, orderedMappedRenderStages, currentResourceStateIt, &synchronizationStage, generateImGuiStage))
					{
						finalStateOfResources[currentResourceStateIt->second.ResourceName] = &currentResourceStateIt->second;
					}
				}
				else
				{
					LOG_ERROR("[RenderGraphParser]: Resource State with attribute index %d could not be found in Resource State Map", resourceStateIdent.AttributeIndex);
					return false;
				}
			}

			RenderStageDesc parsedRenderStage = {};
			CreateParsedRenderStage(resourceStatesByHalfAttributeIndex, &parsedRenderStage, pCurrentRenderStage);

			orderedRenderStages.PushBack(parsedRenderStage);
			orderedPipelineStages.PushBack({ ERenderGraphPipelineStageType::RENDER, uint32(orderedRenderStages.GetSize()) - 1 });

			if (synchronizationStage.Synchronizations.GetSize() > 0)
			{
				orderedSynchronizationStages.PushBack(synchronizationStage);
				orderedPipelineStages.PushBack({ ERenderGraphPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.GetSize()) - 1 });
			}
		}

		if (generateImGuiStage)
		{
			SynchronizationStageDesc imguiSynchronizationStage = {};

			for (auto resourceStateIt = finalStateOfResources.begin(); resourceStateIt != finalStateOfResources.end(); resourceStateIt++)
			{
				const EditorRenderGraphResourceState* pFinalResourceState = resourceStateIt->second;

				auto finalResourceIt = std::find_if(resources.Begin(), resources.End(), [resourceStateIt](const RenderGraphResourceDesc& resourceDesc) { return resourceStateIt->second->ResourceName == resourceDesc.Name; });

				if (finalResourceIt != resources.end())
				{
					if (CapturedByImGui(finalResourceIt))
					{
						//Todo: What if SubResourceCount > 1
						//The Back Buffer is manually added below
						if (pFinalResourceState->ResourceName != RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
						{
							RenderGraphResourceState resourceState = {};
							resourceState.ResourceName = pFinalResourceState->ResourceName;

							//If this resource is not the Back Buffer, we need to check if the following frame needs to have the resource transitioned to some initial state
							for (auto orderedRenderStageIt = orderedMappedRenderStages.rbegin(); orderedRenderStageIt != orderedMappedRenderStages.rend(); orderedRenderStageIt++)
							{
								const EditorRenderStageDesc* pPotentialNextRenderStage = orderedRenderStageIt->second;
								bool done = false;

								if (!done)
								{
									auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(pFinalResourceState->ResourceName);

									if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
									{
										auto nextResourceStateIt = resourceStatesByHalfAttributeIndex.find(potentialNextResourceStateIdentIt->AttributeIndex / 2);

										if (nextResourceStateIt != resourceStatesByHalfAttributeIndex.end())
										{
											RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
											resourceSynchronization.PrevRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
											resourceSynchronization.NextRenderStage = pPotentialNextRenderStage->Name;
											resourceSynchronization.PrevBindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;
											resourceSynchronization.NextBindingType = nextResourceStateIt->second.BindingType;
											resourceSynchronization.PrevQueue		= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
											resourceSynchronization.NextQueue		= ConvertPipelineStateTypeToQueue(pPotentialNextRenderStage->Type);
											resourceSynchronization.ResourceName	= pFinalResourceState->ResourceName;
											resourceSynchronization.ResourceType	= ERenderGraphResourceType::TEXTURE;

											imguiSynchronizationStage.Synchronizations.PushBack(resourceSynchronization);

											done = true;
											break;
										}
									}
								}
							}

							resourceState.BindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;
							imguiRenderStage.ResourceStates.PushBack(resourceState);
						}
					}
				}
				else
				{
					LOG_ERROR("[RenderGraphEditor]: Final Resource State with name \"%s\" could not be found among resources", pFinalResourceState->ResourceName.c_str());
					return false;
				}
			}

			//Forcefully add the Back Buffer as a Render Pass Attachment
			{
				//This is just a dummy as it will be removed in a later stage
				RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
				resourceSynchronization.PrevRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
				resourceSynchronization.NextRenderStage = "PRESENT (Not a Render Stage)";
				resourceSynchronization.PrevBindingType = ERenderGraphResourceBindingType::ATTACHMENT;
				resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::PRESENT;
				resourceSynchronization.PrevQueue		= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
				resourceSynchronization.NextQueue		= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
				resourceSynchronization.ResourceName	= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
				resourceSynchronization.ResourceType	= ERenderGraphResourceType::TEXTURE;

				imguiSynchronizationStage.Synchronizations.PushBack(resourceSynchronization);

				RenderGraphResourceState resourceState = {};
				resourceState.ResourceName	= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
				resourceState.BindingType	= ERenderGraphResourceBindingType::ATTACHMENT;

				imguiRenderStage.ResourceStates.PushBack(resourceState);
			}

			orderedRenderStages.PushBack(imguiRenderStage);
			orderedPipelineStages.PushBack({ ERenderGraphPipelineStageType::RENDER, uint32(orderedRenderStages.GetSize()) - 1 });

			orderedSynchronizationStages.PushBack(imguiSynchronizationStage);
			orderedPipelineStages.PushBack({ ERenderGraphPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.GetSize()) - 1 });
		}

		//Do an extra pass to remove unnecessary synchronizations
		for (auto pipelineStageIt = orderedPipelineStages.begin(); pipelineStageIt != orderedPipelineStages.end();)
		{
			const PipelineStageDesc* pPipelineStageDesc = &(*pipelineStageIt);

			if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
			{
				SynchronizationStageDesc* pSynchronizationStage = &orderedSynchronizationStages[pPipelineStageDesc->StageIndex];

				for (auto synchronizationIt = pSynchronizationStage->Synchronizations.begin(); synchronizationIt != pSynchronizationStage->Synchronizations.end();)
				{
					if (ResourceStatesSynchronizationallyEqual(
						synchronizationIt->ResourceType,
						synchronizationIt->PrevQueue, 
						synchronizationIt->NextQueue,
						synchronizationIt->PrevBindingType,
						synchronizationIt->NextBindingType))
					{
						synchronizationIt = pSynchronizationStage->Synchronizations.Erase(synchronizationIt);
						continue;
					}

					synchronizationIt++;
				}

				if (pSynchronizationStage->Synchronizations.IsEmpty())
				{
					//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
					for (auto updatePipelineStageIt = pipelineStageIt + 1; updatePipelineStageIt != orderedPipelineStages.end(); updatePipelineStageIt++)
					{
						if (updatePipelineStageIt->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
						{
							updatePipelineStageIt->StageIndex--;
						}
					}

					orderedSynchronizationStages.Erase(orderedSynchronizationStages.begin() + pPipelineStageDesc->StageIndex);
					pipelineStageIt = orderedPipelineStages.Erase(pipelineStageIt);

					continue;
				}
			}

			pipelineStageIt++;
		}

		//Do a pass to convert Barriers synchronizations to Render Pass transitions, where applicable
		for (int32 p = 0; p < int32(orderedPipelineStages.GetSize()); p++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &orderedPipelineStages[p];

			if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER)
			{
				RenderStageDesc* pRenderStageDesc = &orderedRenderStages[pPipelineStageDesc->StageIndex];

				//Check if this Render Stage is a Graphics Stage, if it is, we can look for Render Pass Attachments
				if (pRenderStageDesc->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
				{
					for (auto resourceStateIt = pRenderStageDesc->ResourceStates.begin(); resourceStateIt != pRenderStageDesc->ResourceStates.end(); resourceStateIt++)
					{
						RenderGraphResourceState* pResourceState = &(*resourceStateIt);

						bool isBackBuffer = pResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

						//Check if this Resource State has a binding type of ATTACHMENT, if it does, we need to modify the surrounding barriers and the internal Previous- and Next States of the Resource State
						if (pResourceState->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
						{
							RenderGraphResourceState* pPreviousResourceStateDesc = nullptr;
							int32														previousSynchronizationPipelineStageDescIndex = -1;
							TArray<RenderGraphResourceSynchronizationDesc>::Iterator	previousSynchronizationDescIt;

							RenderGraphResourceState* pNextResourceStateDesc = nullptr;
							int32														nextSynchronizationPipelineStageDescIndex = -1;
							TArray<RenderGraphResourceSynchronizationDesc>::Iterator	nextSynchronizationDescIt;

							//Find Previous Synchronization Stage that contains a Synchronization for this resource
							for (int32 pp = p - 1; pp != p; pp--)
							{
								//Loop around if needed
								if (pp < 0)
								{
									//Back buffer is not allowed to loop around
									if (isBackBuffer)
									{
										break;
									}
									else
									{
										pp = orderedPipelineStages.GetSize() - 1;

										if (pp == p)
											break;
									}
								}

								const PipelineStageDesc* pPreviousPipelineStageDesc = &orderedPipelineStages[pp];

								if (pPreviousPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION && previousSynchronizationPipelineStageDescIndex == -1)
								{
									SynchronizationStageDesc* pPotentialPreviousSynchronizationStageDesc = &orderedSynchronizationStages[pPreviousPipelineStageDesc->StageIndex];

									for (auto prevSynchronizationIt = pPotentialPreviousSynchronizationStageDesc->Synchronizations.begin(); prevSynchronizationIt != pPotentialPreviousSynchronizationStageDesc->Synchronizations.end(); prevSynchronizationIt++)
									{
										RenderGraphResourceSynchronizationDesc* pSynchronizationDesc = &(*prevSynchronizationIt);

										if (pSynchronizationDesc->ResourceName == pResourceState->ResourceName)
										{
											previousSynchronizationPipelineStageDescIndex = pp;
											previousSynchronizationDescIt = prevSynchronizationIt;
											break;
										}
									}
								}
								else if (pPreviousPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER && pPreviousResourceStateDesc == nullptr)
								{
									RenderStageDesc* pPotentialPreviousRenderStageDesc = &orderedRenderStages[pPreviousPipelineStageDesc->StageIndex];

									for (auto prevResourceStateIt = pPotentialPreviousRenderStageDesc->ResourceStates.begin(); prevResourceStateIt != pPotentialPreviousRenderStageDesc->ResourceStates.end(); prevResourceStateIt++)
									{
										RenderGraphResourceState* pPotentialPreviousResourceState = &(*prevResourceStateIt);

										if (pPotentialPreviousResourceState->ResourceName == pResourceState->ResourceName)
										{
											pPreviousResourceStateDesc = pPotentialPreviousResourceState;
											break;
										}
									}
								}

								if (previousSynchronizationPipelineStageDescIndex != -1 && pPreviousResourceStateDesc != nullptr)
									break;
							}

							//Find Next Synchronization Stage that contains a Synchronization for this resource
							for (int32 np = p + 1; np != p; np++)
							{
								//Loop around if needed
								if (np >= int32(orderedPipelineStages.GetSize()))
								{
									//Back buffer is not allowed to loop around
									if (isBackBuffer)
									{
										break;
									}
									else
									{
										np = 0;

										if (np == p)
											break;
									}
								}

								const PipelineStageDesc* pNextPipelineStageDesc = &orderedPipelineStages[np];

								if (pNextPipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION && nextSynchronizationPipelineStageDescIndex == -1)
								{
									SynchronizationStageDesc* pPotentialNextSynchronizationStageDesc = &orderedSynchronizationStages[pNextPipelineStageDesc->StageIndex];

									for (auto nextSynchronizationIt = pPotentialNextSynchronizationStageDesc->Synchronizations.begin(); nextSynchronizationIt != pPotentialNextSynchronizationStageDesc->Synchronizations.end(); nextSynchronizationIt++)
									{
										RenderGraphResourceSynchronizationDesc* pSynchronizationDesc = &(*nextSynchronizationIt);

										if (pSynchronizationDesc->ResourceName == pResourceState->ResourceName)
										{
											nextSynchronizationPipelineStageDescIndex = np;
											nextSynchronizationDescIt = nextSynchronizationIt;
											break;
										}
									}
								}
								else if (pNextPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER && pNextResourceStateDesc == nullptr)
								{
									RenderStageDesc* pPotentialNextRenderStageDesc = &orderedRenderStages[pNextPipelineStageDesc->StageIndex];

									for (auto nextResourceStateIt = pPotentialNextRenderStageDesc->ResourceStates.begin(); nextResourceStateIt != pPotentialNextRenderStageDesc->ResourceStates.end(); nextResourceStateIt++)
									{
										RenderGraphResourceState* pPotentialNextResourceState = &(*nextResourceStateIt);

										if (pPotentialNextResourceState->ResourceName == pResourceState->ResourceName)
										{
											pNextResourceStateDesc = pPotentialNextResourceState;
											break;
										}
									}
								}

								if (nextSynchronizationPipelineStageDescIndex != -1 && pNextResourceStateDesc != nullptr)
									break;
							}

							if (pPreviousResourceStateDesc != nullptr)
							{
								pResourceState->AttachmentSynchronizations.PrevBindingType	= pPreviousResourceStateDesc->BindingType;
							}
							else
							{
								pResourceState->AttachmentSynchronizations.PrevBindingType	= ERenderGraphResourceBindingType::NONE;
							}

							if (pNextResourceStateDesc != nullptr)
							{
								pResourceState->AttachmentSynchronizations.NextBindingType = pNextResourceStateDesc->BindingType;
							}
							else
							{
								if (isBackBuffer)
								{
									pResourceState->AttachmentSynchronizations.NextBindingType = ERenderGraphResourceBindingType::PRESENT;
								}
								else
								{
									auto resourceIt = std::find_if(resources.Begin(), resources.End(), [pResourceState](const RenderGraphResourceDesc& resourceDesc) { return pResourceState->ResourceName == resourceDesc.Name; });

									if (resourceIt != resources.end() && resourceIt->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
									{
										pResourceState->AttachmentSynchronizations.NextBindingType = ERenderGraphResourceBindingType::ATTACHMENT;
									}
									else
									{
										LOG_ERROR("[RenderGraphEditor]: Resource \"%s\" is used as an attachment in Render Stage \"%s\" but is not used in later stages", pResourceState->ResourceName.c_str(), pRenderStageDesc->Name.c_str());
										return false;
									}
								}
							}

							if (previousSynchronizationPipelineStageDescIndex != -1)
							{
								SynchronizationStageDesc* pPreviousSynchronizationStage = &orderedSynchronizationStages[orderedPipelineStages[previousSynchronizationPipelineStageDescIndex].StageIndex];

								//If this is a queue transfer, the barrier must remain but the state change should be handled by the Render Pass, otherwise remove it
								if (previousSynchronizationDescIt->PrevQueue != previousSynchronizationDescIt->NextQueue)
								{
									previousSynchronizationDescIt->NextBindingType = previousSynchronizationDescIt->PrevBindingType;
								}
								else
								{
									pPreviousSynchronizationStage->Synchronizations.Erase(previousSynchronizationDescIt);

									if (pPreviousSynchronizationStage->Synchronizations.IsEmpty())
									{
										//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
										for (int32 up = previousSynchronizationPipelineStageDescIndex + 1; up < int32(orderedPipelineStages.GetSize()); up++)
										{
											PipelineStageDesc* pUpdatePipelineStageDesc = &orderedPipelineStages[up];

											if (pUpdatePipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
											{
												pUpdatePipelineStageDesc->StageIndex--;
											}
										}

										if (nextSynchronizationPipelineStageDescIndex > previousSynchronizationPipelineStageDescIndex)
										{
											nextSynchronizationPipelineStageDescIndex--;
										}

										orderedSynchronizationStages.Erase(orderedSynchronizationStages.begin() + orderedPipelineStages[previousSynchronizationPipelineStageDescIndex].StageIndex);
										orderedPipelineStages.Erase(orderedPipelineStages.begin() + previousSynchronizationPipelineStageDescIndex);
										p--;
									}
								}
							}

							if (nextSynchronizationPipelineStageDescIndex != -1 && previousSynchronizationPipelineStageDescIndex != nextSynchronizationPipelineStageDescIndex)
							{
								SynchronizationStageDesc* pNextSynchronizationStage = &orderedSynchronizationStages[orderedPipelineStages[nextSynchronizationPipelineStageDescIndex].StageIndex];

								//If this is a queue transfer, the barrier must remain but the state change should be handled by the Render Pass, otherwise remove it
								if (nextSynchronizationDescIt->PrevQueue != nextSynchronizationDescIt->NextQueue)
								{
									nextSynchronizationDescIt->PrevBindingType = nextSynchronizationDescIt->NextBindingType;
								}
								else
								{
									pNextSynchronizationStage->Synchronizations.Erase(nextSynchronizationDescIt);

									if (pNextSynchronizationStage->Synchronizations.IsEmpty())
									{
										//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
										for (int32 up = nextSynchronizationPipelineStageDescIndex + 1; up < int32(orderedPipelineStages.GetSize()); up++)
										{
											PipelineStageDesc* pUpdatePipelineStageDesc = &orderedPipelineStages[up];

											if (pUpdatePipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
											{
												pUpdatePipelineStageDesc->StageIndex--;
											}
										}

										orderedSynchronizationStages.Erase(orderedSynchronizationStages.begin() + orderedPipelineStages[nextSynchronizationPipelineStageDescIndex].StageIndex);
										orderedPipelineStages.Erase(orderedPipelineStages.begin() + nextSynchronizationPipelineStageDescIndex);
									}
								}
							}
						}
					}
				}
			}
		}

		//Copy Resources so that we can modify them
		pParsedStructure->ResourceDescriptions = resources;

		//Do another pass over all Render Stages to set Resource Flags
		for (uint32 r = 0; r < orderedRenderStages.GetSize(); r++)
		{
			RenderStageDesc* pRenderStageDesc = &orderedRenderStages[r];

			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.GetSize(); rs++)
			{
				RenderGraphResourceState* pResourceState = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = std::find_if(pParsedStructure->ResourceDescriptions.Begin(), pParsedStructure->ResourceDescriptions.End(), [pResourceState](const RenderGraphResourceDesc& resourceDesc) { return pResourceState->ResourceName == resourceDesc.Name; });

				if (resourceIt != pParsedStructure->ResourceDescriptions.end())
				{
					switch (resourceIt->Type)
					{
					case ERenderGraphResourceType::TEXTURE:
					{
						switch (pResourceState->BindingType)
						{
						case ERenderGraphResourceBindingType::COMBINED_SAMPLER:
							resourceIt->TextureParams.TextureFlags |= FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE;
							resourceIt->TextureParams.TextureViewFlags |= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
							break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:
							resourceIt->TextureParams.TextureFlags |= FTextureFlag::TEXTURE_FLAG_UNORDERED_ACCESS;
							resourceIt->TextureParams.TextureViewFlags |= FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS;
							break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:
							resourceIt->TextureParams.TextureFlags |= FTextureFlag::TEXTURE_FLAG_UNORDERED_ACCESS;
							resourceIt->TextureParams.TextureViewFlags |= FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS;
							break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:
							resourceIt->TextureParams.TextureFlags |= FTextureFlag::TEXTURE_FLAG_UNORDERED_ACCESS;
							resourceIt->TextureParams.TextureViewFlags |= FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS;
							break;
						case ERenderGraphResourceBindingType::ATTACHMENT:
						{
							bool isDepthStencilAttachment = resourceIt->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT;
							resourceIt->TextureParams.TextureFlags |= (isDepthStencilAttachment ? FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL : FTextureFlag::TEXTURE_FLAG_RENDER_TARGET);
							resourceIt->TextureParams.TextureViewFlags |= (isDepthStencilAttachment ? FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL : FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET);
							break;
						}
						}

						if (resourceIt->TextureParams.TextureType == ERenderGraphTextureType::TEXTURE_CUBE)
							resourceIt->TextureParams.TextureFlags |= FTextureFlag::TEXTURE_FLAG_CUBE_COMPATIBLE;

						break;
					}
					case ERenderGraphResourceType::SCENE_DRAW_ARGS:
					case ERenderGraphResourceType::BUFFER:
					{
						switch (pResourceState->BindingType)
						{
						case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				resourceIt->BufferParams.BufferFlags |= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER; break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		resourceIt->BufferParams.BufferFlags |= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		resourceIt->BufferParams.BufferFlags |= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
						case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	resourceIt->BufferParams.BufferFlags |= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
						}
						break;
					}
					case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
					{
						break;
					}
					}
				}
			}
		}

		//Remove Resources not ever used
		for (auto resourceIt = pParsedStructure->ResourceDescriptions.Begin(); resourceIt != pParsedStructure->ResourceDescriptions.End();)
		{
			bool resourceIsUsed = std::find_if(resourceNamesActuallyUsed.begin(), resourceNamesActuallyUsed.end(), [resourceIt](const String& resourceName) { return resourceName == resourceIt->Name; }) != resourceNamesActuallyUsed.end();

			if (!resourceIsUsed)
			{
				resourceIt = pParsedStructure->ResourceDescriptions.Erase(resourceIt);
			}
			else
			{
				resourceIt++;
			}
		}

		auto backBufferResource = std::find_if(pParsedStructure->ResourceDescriptions.Begin(), pParsedStructure->ResourceDescriptions.End(), [](const RenderGraphResourceDesc& resourceDesc) { return resourceDesc.Name == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT; });

		if (backBufferResource == pParsedStructure->ResourceDescriptions.end())
		{
			pParsedStructure->ResourceDescriptions.PushBack(CreateBackBufferResource());
		}

		pParsedStructure->RenderStageDescriptions			= orderedRenderStages;
		pParsedStructure->SynchronizationStageDescriptions	= orderedSynchronizationStages;
		pParsedStructure->PipelineStageDescriptions			= orderedPipelineStages;

		return true;
	}

	RenderGraphResourceDesc RenderGraphParser::CreateBackBufferResource()
	{
		RenderGraphResourceDesc resource = {};
		resource.Name							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
		resource.Type							= ERenderGraphResourceType::TEXTURE;
		resource.SubResourceCount				= 1;
		resource.Editable						= false;
		resource.External						= false;
		resource.TextureParams.TextureFormat	= EFormat::FORMAT_B8G8R8A8_UNORM;
		return resource;
	}

	bool RenderGraphParser::RecursivelyWeightParentRenderStages(
		THashTable<String, EditorRenderStageDesc>::const_iterator childRenderStageIt,
		const THashTable<String, EditorRenderStageDesc>& renderStagesByName,
		const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
		const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex,
		THashTable<String, int32>& renderStageWeightsByName,
		uint32 currentDrawArgsMask)
	{
		TSet<String> parentRenderStageNames;
		bool result = true;

		//Todo: Implement Index Buffer & Indirect Args Buffer 

		//Iterate through all resource states in the current Render Stages
		for (const EditorResourceStateIdent& resourceStateIdent : childRenderStageIt->second.ResourceStateIdents)
		{
			auto resourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceStateIdent.AttributeIndex / 2);

			//Check if resource state has input link
			if (resourceStateIt != resourceStatesByHalfAttributeIndex.end())
			{
				if (resourceStateIt->second.InputLinkIndex != -1)
				{
					auto resourceLinkIt = resourceStateLinksByLinkIndex.find(resourceStateIt->second.InputLinkIndex);

					if (resourceLinkIt != resourceStateLinksByLinkIndex.end())
					{
						auto prevResourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceLinkIt->second.SrcAttributeIndex / 2);

						if (prevResourceStateIt != resourceStatesByHalfAttributeIndex.end())
						{
							uint32 nextDrawArgsMask = currentDrawArgsMask & prevResourceStateIt->second.DrawArgsMask;

							//Check if Draw Buffers, if it, check if mask are overlapping
							if (resourceStateIt->second.ResourceType == ERenderGraphResourceType::SCENE_DRAW_ARGS && nextDrawArgsMask == 0)
								continue;

							//Make sure parent is a Render Stage and check if it has already been visited
							if (renderStagesByName.count(prevResourceStateIt->second.RenderStageName) > 0 && parentRenderStageNames.count(prevResourceStateIt->second.RenderStageName) == 0)
							{
								auto parentRenderStageIt = renderStagesByName.find(prevResourceStateIt->second.RenderStageName);

								if (parentRenderStageIt != renderStagesByName.end())
								{
									result = result && RecursivelyWeightParentRenderStages(
										parentRenderStageIt,
										renderStagesByName,
										resourceStatesByHalfAttributeIndex,
										resourceStateLinksByLinkIndex,
										renderStageWeightsByName,
										nextDrawArgsMask);


									parentRenderStageNames.insert(parentRenderStageIt->first);

									auto parentRenderStageWeightIt = renderStageWeightsByName.find(prevResourceStateIt->second.RenderStageName);

									if (parentRenderStageWeightIt != renderStageWeightsByName.end())
									{
										parentRenderStageWeightIt->second++;
									}
									else
									{
										result = false;
									}
								}
								else
								{
									result = false;
								}
							}
						}
						else
						{
							result = false;
						}
					}
					else
					{
						result = false;
					}
				}
			}
			else
			{
				result = false;
			}
		}

		return result;
	}

	bool RenderGraphParser::FindAndCreateSynchronization(
		const TArray<RenderGraphResourceDesc>& resources,
		const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex,
		std::multimap<uint32, const EditorRenderStageDesc*>::reverse_iterator currentOrderedRenderStageIt,
		const std::multimap<uint32, const EditorRenderStageDesc*>& orderedMappedRenderStages,
		THashTable<int32, EditorRenderGraphResourceState>::const_iterator currentResourceStateIt,
		SynchronizationStageDesc* pSynchronizationStage,
		bool generateImGuiStage)
	{
		const EditorRenderStageDesc* pCurrentRenderStage				= currentOrderedRenderStageIt->second;

		const EditorRenderGraphResourceState* pNextResourceState		= nullptr;
		const EditorRenderStageDesc* pNextRenderStage					= nullptr;

		//Loop through the following Render Stages and find the first one that uses this Resource
		auto nextOrderedRenderStageIt = currentOrderedRenderStageIt;
		nextOrderedRenderStageIt++;
		for (; nextOrderedRenderStageIt != orderedMappedRenderStages.rend(); nextOrderedRenderStageIt++)
		{
			const EditorRenderStageDesc* pPotentialNextRenderStage = nextOrderedRenderStageIt->second;

			//See if this Render Stage uses Resource we are looking for
			auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(currentResourceStateIt->second.ResourceName);

			if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
			{
				auto nextResourceStateIt = resourceStatesByHalfAttributeIndex.find(potentialNextResourceStateIdentIt->AttributeIndex / 2);

				if (nextResourceStateIt != resourceStatesByHalfAttributeIndex.end())
				{
					if (currentResourceStateIt->second.ResourceType != ERenderGraphResourceType::SCENE_DRAW_ARGS || (currentResourceStateIt->second.DrawArgsMask & nextResourceStateIt->second.DrawArgsMask) > 0)
					{
						pNextResourceState = &nextResourceStateIt->second;
						pNextRenderStage = pPotentialNextRenderStage;
						break;
					}
				}
			}
		}

		//If there is a Next State for the Resource, pNextResourceState will not be nullptr 
		RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
		resourceSynchronization.PrevRenderStage = pCurrentRenderStage->Name;
		resourceSynchronization.ResourceName	= currentResourceStateIt->second.ResourceName;
		
		resourceSynchronization.PrevQueue		= ConvertPipelineStateTypeToQueue(pCurrentRenderStage->Type);
		resourceSynchronization.PrevBindingType	= currentResourceStateIt->second.BindingType;

		bool isBackBuffer = currentResourceStateIt->second.ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

		auto resourceIt = std::find_if(resources.Begin(), resources.End(), [currentResourceStateIt](const RenderGraphResourceDesc& resourceDesc) { return currentResourceStateIt->second.ResourceName == resourceDesc.Name; });

		if (resourceIt != resources.end())
		{
			resourceSynchronization.ResourceType = resourceIt->Type;

			if (pNextResourceState != nullptr)
			{
				//If the ResourceState is Readonly and the current and next Binding Types are the same we don't want a synchronization, no matter what queue type
				if (!(IsReadOnly(currentResourceStateIt->second.BindingType) && currentResourceStateIt->second.BindingType == pNextResourceState->BindingType))
				{
					//Check if pNextResourceState belongs to a Render Stage, otherwise we need to check if it belongs to Final Output
					if (pNextRenderStage != nullptr)
					{
						resourceSynchronization.NextRenderStage = pNextRenderStage->Name;
						resourceSynchronization.NextQueue		= ConvertPipelineStateTypeToQueue(pNextRenderStage->Type);
						resourceSynchronization.NextBindingType = pNextResourceState->BindingType;
						resourceSynchronization.DrawArgsMask	= currentResourceStateIt->second.DrawArgsMask & pNextResourceState->DrawArgsMask;

						pSynchronizationStage->Synchronizations.PushBack(resourceSynchronization);
					}
				}
			}
			else if (generateImGuiStage && CapturedByImGui(resourceIt))
			{
				if (isBackBuffer)
				{
					resourceSynchronization.NextRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
					resourceSynchronization.NextQueue		= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
					resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::ATTACHMENT;

					pSynchronizationStage->Synchronizations.PushBack(resourceSynchronization);
				}
				else if (!(IsReadOnly(currentResourceStateIt->second.BindingType) && currentResourceStateIt->second.BindingType == ERenderGraphResourceBindingType::COMBINED_SAMPLER))
				{
					//Todo: What if Subresource Count > 1
					//Capture resource synchronizations here, even for Back Buffer, PRESENT Synchronization is seperately solved later
					resourceSynchronization.NextRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
					resourceSynchronization.NextQueue		= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
					resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;

					pSynchronizationStage->Synchronizations.PushBack(resourceSynchronization);
				}
			}
			else if (isBackBuffer)
			{
				resourceSynchronization.NextQueue = ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
				resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::PRESENT;

				pSynchronizationStage->Synchronizations.PushBack(resourceSynchronization);
			}
			else
			{
				//If there is no following Render Stage that uses the Resource and it is not captured by ImGui and it is not the back buffer, we need to check the previous ones to discover synchronizations for the next frame
				for (auto previousOrderedRenderStageIt = orderedMappedRenderStages.rbegin(); previousOrderedRenderStageIt != currentOrderedRenderStageIt; previousOrderedRenderStageIt++)
				{
					const EditorRenderStageDesc* pPotentialNextRenderStage = previousOrderedRenderStageIt->second;

					//See if this Render Stage uses Resource we are looking for
					auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(currentResourceStateIt->second.ResourceName);

					if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
					{
						auto nextResourceStateIt = resourceStatesByHalfAttributeIndex.find(potentialNextResourceStateIdentIt->AttributeIndex / 2);

						if (nextResourceStateIt != resourceStatesByHalfAttributeIndex.end())
						{
							if (currentResourceStateIt->second.ResourceType != ERenderGraphResourceType::SCENE_DRAW_ARGS || (currentResourceStateIt->second.DrawArgsMask & nextResourceStateIt->second.DrawArgsMask) > 0)
							{
								pNextResourceState = &nextResourceStateIt->second;
								pNextRenderStage = pPotentialNextRenderStage;
								break;
							}
						}
					}
				}

				//It is safe to add this synchronization here, since we know that the resource will not be captured by ImGui
				if (pNextResourceState != nullptr)
				{
					//If the ResourceState is Readonly and the current and next Binding Types are the same we dont want a synchronization, no matter what queue type
					if (!IsReadOnly(currentResourceStateIt->second.BindingType) || currentResourceStateIt->second.BindingType != pNextResourceState->BindingType)
					{
						//Check if pNextResourceState belongs to a Render Stage, otherwise we need to check if it belongs to Final Output
						if (pNextRenderStage != nullptr)
						{
							resourceSynchronization.NextRenderStage = pNextRenderStage->Name;
							resourceSynchronization.NextQueue		= ConvertPipelineStateTypeToQueue(pNextRenderStage->Type);
							resourceSynchronization.NextBindingType = pNextResourceState->BindingType;
							resourceSynchronization.DrawArgsMask	= currentResourceStateIt->second.DrawArgsMask & pNextResourceState->DrawArgsMask;

							pSynchronizationStage->Synchronizations.PushBack(resourceSynchronization);
						}
					}
				}
			}
		}
		else
		{ 
			LOG_ERROR("[RenderGraphEditor]: Resource State with name \"%s\" could not be found among resources when creating Synchronization", currentResourceStateIt->second.ResourceName.c_str());
			return false;
		}

		return true;
	}

	bool RenderGraphParser::CapturedByImGui(TArray<RenderGraphResourceDesc>::ConstIterator resourceIt)
	{
		return resourceIt->Type == ERenderGraphResourceType::TEXTURE && resourceIt->SubResourceCount == 1;
	}

	void RenderGraphParser::CreateParsedRenderStage(
		const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex, 
		RenderStageDesc* pDstRenderStage, 
		const EditorRenderStageDesc* pSrcRenderStage)
	{
		pDstRenderStage->Name					= pSrcRenderStage->Name;
		pDstRenderStage->Type					= pSrcRenderStage->Type;
		pDstRenderStage->CustomRenderer			= pSrcRenderStage->CustomRenderer;
		pDstRenderStage->Parameters				= pSrcRenderStage->Parameters;

		pDstRenderStage->TriggerType			= pSrcRenderStage->TriggerType;
		pDstRenderStage->FrameDelay				= pSrcRenderStage->FrameDelay;
		pDstRenderStage->FrameOffset			= pSrcRenderStage->FrameOffset;

		pDstRenderStage->ResourceStates.Reserve(pSrcRenderStage->ResourceStateIdents.GetSize());

		for (const EditorResourceStateIdent& resourceStateIdent : pSrcRenderStage->ResourceStateIdents)
		{
			auto resourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceStateIdent.AttributeIndex / 2);
			
			if (resourceStateIt != resourceStatesByHalfAttributeIndex.end())
			{
				RenderGraphResourceState resourceState = {};
				resourceState.ResourceName								= resourceStateIt->second.ResourceName;
				resourceState.BindingType								= resourceStateIt->second.BindingType;
				resourceState.DrawArgsMask							= resourceStateIt->second.DrawArgsMask;
				resourceState.AttachmentSynchronizations.PrevSameFrame	= resourceStateIt->second.InputLinkIndex != -1;

				pDstRenderStage->ResourceStates.PushBack(resourceState);
			}
		}

		if (pDstRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
		{
			pDstRenderStage->Graphics.Shaders					= pSrcRenderStage->Graphics.Shaders;
			pDstRenderStage->Graphics.DrawType					= pSrcRenderStage->Graphics.DrawType;
			pDstRenderStage->Graphics.DepthTestEnabled			= pSrcRenderStage->Graphics.DepthTestEnabled;
			pDstRenderStage->Graphics.CullMode					= pSrcRenderStage->Graphics.CullMode;
			pDstRenderStage->Graphics.PolygonMode				= pSrcRenderStage->Graphics.PolygonMode;
			pDstRenderStage->Graphics.PrimitiveTopology			= pSrcRenderStage->Graphics.PrimitiveTopology;
		}
		else if (pDstRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
		{
			pDstRenderStage->Compute.ShaderName					= pSrcRenderStage->Compute.ShaderName;
		}
		else if (pDstRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
		{
			pDstRenderStage->RayTracing.Shaders					= pSrcRenderStage->RayTracing.Shaders;
		}
	}
}