#include "Rendering/RenderGraph.h"

#include "Log/Log.h"

#include <cstdio>

namespace LambdaEngine
{
	RenderGraph::RenderGraph()
	{
	}

	RenderGraph::~RenderGraph()
	{
	}

	bool RenderGraph::Init(const RenderGraphDesc& desc)
	{
		std::unordered_map<const char*, InternalRenderStage>						internalRenderStages;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			internalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			internalTemporalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment> internalExternalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>		internalOutputAttachments;

		uint32 globalAttachmentIndex = 0;

		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStage& renderStage = desc.pRenderStages[renderStageIndex];

			auto renderStageIt = internalRenderStages.find(renderStage.pName);

			if (renderStageIt != internalRenderStages.end())
			{
				LOG_ERROR("[RenderGraph]: Multiple Render Stages with same name is not allowed, name: \"%s\"", renderStage.pName);
				return false;
			}

			InternalRenderStage& internalRenderStage = internalRenderStages[renderStage.pName];
			internalRenderStage.pRenderStage = &renderStage;
			internalRenderStage.GlobalIndex = renderStageIndex;

			//Input Attachments
			for (uint32 inputAttachmentIndex = 0; inputAttachmentIndex < renderStage.InputAttachmentCount; inputAttachmentIndex++)
			{
				const RenderStageInputAttachment& renderStageInputAttachment = renderStage.pInputAttachments[inputAttachmentIndex];
				auto inputIt = internalInputAttachments.find(renderStageInputAttachment.pName);
				auto temporalInputIt = internalTemporalInputAttachments.find(renderStageInputAttachment.pName);

				bool isTemporal = IsInputTemporal(renderStage, &renderStageInputAttachment);

				if (isTemporal)
				{
					//Temporal
					if (temporalInputIt != internalTemporalInputAttachments.end() &&
						temporalInputIt->second.pAttachment->Type == renderStageInputAttachment.Type)
					{
						temporalInputIt->second.RenderStages.push_back(&internalRenderStage);
						internalRenderStage.TemporalInputAttachments.push_back(&temporalInputIt->second);
					}
					else
					{
						InternalRenderStageInputAttachment& internalTemporalInputAttachment = internalTemporalInputAttachments[renderStageInputAttachment.pName];
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
					if (inputIt != internalInputAttachments.end() &&
						inputIt->second.pAttachment->Type == renderStageInputAttachment.Type)
					{
						inputIt->second.RenderStages.push_back(&internalRenderStage);
						internalRenderStage.InputAttachments.push_back(&inputIt->second);
					}
					else
					{
						InternalRenderStageInputAttachment& internalInputAttachment = internalInputAttachments[renderStageInputAttachment.pName];
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
				auto externalInputIt = internalExternalInputAttachments.find(renderStageExternalInputAttachment.pName);

				if (externalInputIt != internalExternalInputAttachments.end() && 
					externalInputIt->second.pAttachment->Type == renderStageExternalInputAttachment.Type && 
					externalInputIt->second.pAttachment->DescriptorCount == renderStageExternalInputAttachment.DescriptorCount)
				{
					externalInputIt->second.RenderStages.push_back(&internalRenderStage);
					internalRenderStage.ExternalInputAttachments.push_back(&externalInputIt->second);
				}
				else
				{
					InternalRenderStageExternalInputAttachment& internalExternalInputAttachment = internalExternalInputAttachments[renderStageExternalInputAttachment.pName];
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
				auto outputIt = internalOutputAttachments.find(renderStageOutputAttachment.pName);

				if (outputIt != internalOutputAttachments.end() &&
					outputIt->second.pAttachment->Type == renderStageOutputAttachment.Type)
				{
					outputIt->second.RenderStages.push_back(&internalRenderStage);
					internalRenderStage.OutputAttachments.push_back(&outputIt->second);
				}
				else
				{
					InternalRenderStageOutputAttachment& internalOutputAttachment = internalOutputAttachments[renderStageOutputAttachment.pName];
					internalOutputAttachment.pAttachment = &renderStageOutputAttachment;
					internalOutputAttachment.RenderStages.push_back(&internalRenderStage);
					internalOutputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalRenderStage.OutputAttachments.push_back(&internalOutputAttachment);
				}
			}
		}

		//Connect Output Attachments to Input Attachments
		for (auto& outputAttachmentPair : internalOutputAttachments)
		{
			InternalRenderStageOutputAttachment& renderStageOutputAttachment = outputAttachmentPair.second;

			for (auto& inputAttachmentPair : internalInputAttachments)
			{
				InternalRenderStageInputAttachment& renderStageInputAttachment = inputAttachmentPair.second;

				if (CompatibleAttachmentNames(renderStageInputAttachment.pAttachment, renderStageOutputAttachment.pAttachment))
				{
					if (CompatibleAttachmentTypes(renderStageInputAttachment.pAttachment, renderStageOutputAttachment.pAttachment))
					{
						renderStageOutputAttachment.pConnectedAttachment = &renderStageInputAttachment;
						break;
					}
				}
			}
		}

		if (desc.CreateDebugGraph)
		{
			char filepathBuffer[1024];
			strcpy(filepathBuffer, desc.pName);
			strcat(filepathBuffer, ".dot");

			FILE* pGraphVizFile = fopen(filepathBuffer, "w");

			if (pGraphVizFile == nullptr)
			{
				LOG_WARNING("[ResourceDevice]: Failed to load file \"%s\"", filepathBuffer);
				return false;
			}

			fputs("digraph G {\n", pGraphVizFile);
			fputs("\trankdir = LR;\n", pGraphVizFile);
			fputs("\tsplines=polyline\n", pGraphVizFile);

			bool declareExternalInputs = true;
			bool linkExternalInputs = false;

			WriteGraphVizDeclarations(pGraphVizFile, declareExternalInputs, internalRenderStages, internalInputAttachments, internalTemporalInputAttachments, internalExternalInputAttachments, internalOutputAttachments);
			WriteGraphVizDefinitions(pGraphVizFile, declareExternalInputs, linkExternalInputs, internalRenderStages, internalInputAttachments, internalTemporalInputAttachments, internalExternalInputAttachments, internalOutputAttachments);

			fputs("}", pGraphVizFile);

			fclose(pGraphVizFile);

			char outputCommand[1024];
			strcpy(outputCommand, "D:\\\\Graphviz\\\\bin\\\\dot \"");
			strcat(outputCommand, desc.pName);
			strcat(outputCommand, ".dot\" -O\"");
			strcat(outputCommand, desc.pName);
			strcat(outputCommand, ".pdf\" -T");
			strcat(outputCommand, "pdf");

			system(outputCommand);
		}
		
		return false;
	}

	bool RenderGraph::IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment* pInputAttachment)
	{
		for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStage.OutputAttachmentCount; outputAttachmentIndex++)
		{
			const RenderStageOutputAttachment& renderStageOutputAttachment = renderStage.pOutputAttachments[outputAttachmentIndex];

			if (CompatibleAttachmentNames(pInputAttachment, &renderStageOutputAttachment))
			{
				if (CompatibleAttachmentTypes(pInputAttachment, &renderStageOutputAttachment))
					return true;
			}
		}

		return false;
	}

	bool RenderGraph::CompatibleAttachmentNames(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment)
	{
		return (strcmp(pInputAttachment->pName, pOutputAttachment->pName) == 0);
	}

	bool RenderGraph::CompatibleAttachmentTypes(const RenderStageInputAttachment* pInputAttachment, const RenderStageOutputAttachment* pOutputAttachment)
	{
		bool connected =
			(pInputAttachment->Type == EInputAttachmentType::BUFFER && pOutputAttachment->Type == EOutputAttachmentType::BUFFER) ||
			(pInputAttachment->Type == EInputAttachmentType::TEXTURE && (pOutputAttachment->Type == EOutputAttachmentType::TEXTURE || pOutputAttachment->Type == EOutputAttachmentType::DEPTH_STENCIL));

		return connected;
	}

	void RenderGraph::WriteGraphVizDeclarations(
		FILE* pFile,
		bool declareExternalInputs,
		const std::unordered_map<const char*, InternalRenderStage>& internalRenderStages,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& temporalInternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments)
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

		for (auto& attachmentPair : temporalInternalInputAttachments)
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

			for (auto& attachmentPair : internalExternalInputAttachments)
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
		for (auto& renderStagePair : internalRenderStages)
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
		for (auto& attachmentPair : internalInputAttachments)
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
		for (auto& attachmentPair : internalOutputAttachments)
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

	void RenderGraph::WriteGraphVizDefinitions(
		FILE* pFile,
		bool externalInputsDeclared,
		bool linkExternalInputs,
		const std::unordered_map<const char*, InternalRenderStage>& internalRenderStages,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& temporalInternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments)
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

		for (auto& renderStagePair : internalRenderStages)
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

			for (auto attachmentPairIt = internalExternalInputAttachments.begin(); attachmentPairIt != internalExternalInputAttachments.end();)
			{
				const InternalRenderStageExternalInputAttachment& renderStageExternalInputAttachment = attachmentPairIt->second;

				sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);
				
				strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);

				attachmentPairIt++;

				if (attachmentPairIt != internalExternalInputAttachments.end())
					strcat(fileOutputBuffer, " -> ");
			}

			strcat(fileOutputBuffer, "[style=invis];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Output Attachments
		for (auto& attachmentPair : internalOutputAttachments)
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
}