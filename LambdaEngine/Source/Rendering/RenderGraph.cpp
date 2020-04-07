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
		std::unordered_map<const char*, InternalRenderStageInputAttachment>			internalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageExternalInputAttachment> internalExternalInputAttachments;
		std::unordered_map<const char*, InternalRenderStageOutputAttachment>		internalOutputAttachments;

		uint32 globalAttachmentIndex = 0;

		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStage& renderStage = desc.pRenderStages[renderStageIndex];

			//Input Attachments
			for (uint32 inputAttachmentIndex = 0; inputAttachmentIndex < renderStage.InputAttachmentCount; inputAttachmentIndex++)
			{
				const RenderStageInputAttachment& renderStageInputAttachment = renderStage.pInputAttachments[inputAttachmentIndex];
				auto it = internalInputAttachments.find(renderStageInputAttachment.pName);

				if (it != internalInputAttachments.end() && 
					it->second.Attachment.Type == renderStageInputAttachment.Type)
				{
					if (IsInputTemporal(renderStage, renderStageInputAttachment))
					{
						InternalRenderStageInputAttachment internalInputAttachment = {};
						internalInputAttachment.Attachment = renderStageInputAttachment;
						internalInputAttachment.RenderStages.push_back({ renderStage, renderStageIndex });
						internalInputAttachment.GlobalIndex = globalAttachmentIndex;
						internalInputAttachment.Temporal = true;
						globalAttachmentIndex++;

						internalInputAttachments[renderStageInputAttachment.pName] = internalInputAttachment;
					}
					else
					{
						it->second.RenderStages.push_back({ renderStage, renderStageIndex });
					}
				}
				else
				{
					InternalRenderStageInputAttachment internalInputAttachment = {};
					internalInputAttachment.Attachment = renderStageInputAttachment;
					internalInputAttachment.RenderStages.push_back({ renderStage, renderStageIndex });
					internalInputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalInputAttachments[renderStageInputAttachment.pName] = internalInputAttachment;
				}
			}

			//External Input Attachments
			for (uint32 externalInputAttachmentIndex = 0; externalInputAttachmentIndex < renderStage.ExtenalInputAttachmentCount; externalInputAttachmentIndex++)
			{
				const RenderStageExternalInputAttachment& renderStageExternalInputAttachment = renderStage.pExternalInputAttachments[externalInputAttachmentIndex];
				auto it = internalExternalInputAttachments.find(renderStageExternalInputAttachment.pName);

				if (it != internalExternalInputAttachments.end() && 
					it->second.Attachment.Type == renderStageExternalInputAttachment.Type && 
					it->second.Attachment.DescriptorCount == renderStageExternalInputAttachment.DescriptorCount)
				{
					it->second.RenderStages.push_back({ renderStage, renderStageIndex });
				}
				else
				{
					InternalRenderStageExternalInputAttachment internalExternalInputAttachment = {};
					internalExternalInputAttachment.Attachment = renderStageExternalInputAttachment;
					internalExternalInputAttachment.RenderStages.push_back({ renderStage, renderStageIndex });
					internalExternalInputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalExternalInputAttachments[renderStageExternalInputAttachment.pName] = internalExternalInputAttachment;
				}
			}

			//Output Attachments
			for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStage.OutputAttachmentCount; outputAttachmentIndex++)
			{
				const RenderStageOutputAttachment& renderStageOutputAttachment = renderStage.pOutputAttachments[outputAttachmentIndex];
				auto it = internalOutputAttachments.find(renderStageOutputAttachment.pName);

				if (it != internalOutputAttachments.end() &&
					it->second.Attachment.Type == renderStageOutputAttachment.Type)
				{
					it->second.RenderStages.push_back({ renderStage, renderStageIndex });
				}
				else
				{
					InternalRenderStageOutputAttachment internalOutputAttachment = {};
					internalOutputAttachment.Attachment = renderStageOutputAttachment;
					internalOutputAttachment.RenderStages.push_back({ renderStage, renderStageIndex });
					internalOutputAttachment.GlobalIndex = globalAttachmentIndex;
					globalAttachmentIndex++;

					internalOutputAttachments[renderStageOutputAttachment.pName] = internalOutputAttachment;
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

				if (CompatibleAttachmentNames(renderStageInputAttachment.Attachment, renderStageOutputAttachment.Attachment))
				{
					if (CompatibleAttachmentTypes(renderStageInputAttachment.Attachment, renderStageOutputAttachment.Attachment))
					{
						if (!renderStageInputAttachment.Temporal)
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

			WriteGraphVizDeclarations(pGraphVizFile, desc, internalInputAttachments, internalExternalInputAttachments, internalOutputAttachments);
			WriteGraphVizDefinitions(pGraphVizFile, desc, internalInputAttachments, internalExternalInputAttachments, internalOutputAttachments);

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

	bool RenderGraph::IsInputTemporal(const RenderStage& renderStage, const RenderStageInputAttachment& inputAttachment)
	{
		for (uint32 outputAttachmentIndex = 0; outputAttachmentIndex < renderStage.OutputAttachmentCount; outputAttachmentIndex++)
		{
			const RenderStageOutputAttachment& renderStageOutputAttachment = renderStage.pOutputAttachments[outputAttachmentIndex];

			if (CompatibleAttachmentNames(inputAttachment, renderStageOutputAttachment) && CompatibleAttachmentTypes(inputAttachment, renderStageOutputAttachment))
				return true;
		}

		return false;
	}

	bool RenderGraph::CompatibleAttachmentNames(const RenderStageInputAttachment& inputAttachment, const RenderStageOutputAttachment& outputAttachment)
	{
		return (strcmp(inputAttachment.pName, outputAttachment.pName) == 0);
	}

	bool RenderGraph::CompatibleAttachmentTypes(const RenderStageInputAttachment& inputAttachment, const RenderStageOutputAttachment& outputAttachment)
	{
		bool connected =
			(inputAttachment.Type == EInputAttachmentType::BUFFER && outputAttachment.Type == EOutputAttachmentType::BUFFER) ||
			(inputAttachment.Type == EInputAttachmentType::TEXTURE && (outputAttachment.Type == EOutputAttachmentType::TEXTURE || outputAttachment.Type == EOutputAttachmentType::DEPTH_STENCIL));

		return connected;
	}

	void RenderGraph::WriteGraphVizDeclarations(
		FILE* pFile,
		const RenderGraphDesc& desc,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments)
	{
		char fileOutputBuffer[256];
		char renderStageBuffer[32];
		char renderStageInputAttachmentBuffer[32];
		char renderStageExternalInputAttachmentBuffer[32];
		char renderStageOutputAttachmentBuffer[32];
		strcpy(renderStageBuffer, "rs");
		strcpy(renderStageInputAttachmentBuffer, "ia");
		strcpy(renderStageExternalInputAttachmentBuffer, "eia");
		strcpy(renderStageOutputAttachmentBuffer, "oa");

		//Render Stages
		for (uint32 renderStageIndex = 0; renderStageIndex < desc.RenderStageCount; renderStageIndex++)
		{
			const RenderStage& renderStage = desc.pRenderStages[renderStageIndex];

			sprintf(renderStageBuffer + 2, "%u", renderStageIndex);

			strcpy(fileOutputBuffer, "\t");
			strcat(fileOutputBuffer, renderStageBuffer);
			strcat(fileOutputBuffer, " [shape=box,label=\"");
			strcat(fileOutputBuffer, renderStage.pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		//Input Attachments
		for (auto& attachmentPair : internalInputAttachments)
		{
			const InternalRenderStageInputAttachment& renderStageInputAttachment = attachmentPair.second;

			sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t");
			strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [label=\"Input\n");
			strcat(fileOutputBuffer, renderStageInputAttachment.Attachment.pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		fputs("\tsubgraph cluster0 {\n", pFile);
		fputs("\t\tnode [style=filled,color=white];\n", pFile);
		fputs("\t\tstyle = filled;\n", pFile);
		fputs("\t\tcolor = lightgrey;\n", pFile);
		fputs("\t\tlabel = \"External Inputs\";\n", pFile);

		//External Input Attachments
		for (auto& attachmentPair : internalExternalInputAttachments)
		{
			const InternalRenderStageExternalInputAttachment& renderStageExternalInputAttachment = attachmentPair.second;

			sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t\t");
			strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);
			strcat(fileOutputBuffer, " [label=\"External Input\n");
			strcat(fileOutputBuffer, renderStageExternalInputAttachment.Attachment.pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}

		fputs("}\n", pFile);

		//Output Attachments
		for (auto& attachmentPair : internalOutputAttachments)
		{
			const InternalRenderStageOutputAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			strcpy(fileOutputBuffer, "\t");
			strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
			strcat(fileOutputBuffer, " [label=\"Output\n");
			strcat(fileOutputBuffer, renderStageOutputAttachment.Attachment.pName);
			strcat(fileOutputBuffer, "\"];\n");
			fputs(fileOutputBuffer, pFile);
		}
	}

	void RenderGraph::WriteGraphVizDefinitions(
		FILE* pFile,
		const RenderGraphDesc& desc,
		const std::unordered_map<const char*, InternalRenderStageInputAttachment>& internalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageExternalInputAttachment>& internalExternalInputAttachments,
		const std::unordered_map<const char*, InternalRenderStageOutputAttachment>& internalOutputAttachments)
	{
		char fileOutputBuffer[256];
		char renderStageBuffer[32];
		char renderStageInputAttachmentBuffer[32];
		char renderStageExternalInputAttachmentBuffer[32];
		char renderStageOutputAttachmentBuffer[32];
		strcpy(renderStageBuffer, "rs");
		strcpy(renderStageInputAttachmentBuffer, "ia");
		strcpy(renderStageExternalInputAttachmentBuffer, "eia");
		strcpy(renderStageOutputAttachmentBuffer, "oa");

		//Input Attachments
		for (auto& attachmentPair : internalInputAttachments)
		{
			const InternalRenderStageInputAttachment& renderStageInputAttachment = attachmentPair.second;

			sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageInputAttachment.GlobalIndex);

			for (auto renderStageIt : renderStageInputAttachment.RenderStages)
			{
				sprintf(renderStageBuffer + 2, "%u", renderStageIt.GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, ";\n");
				fputs(fileOutputBuffer, pFile);
			}
		}

		//External Input Attachments
		for (auto& attachmentPair : internalExternalInputAttachments)
		{
			const InternalRenderStageExternalInputAttachment& renderStageExternalInputAttachment = attachmentPair.second;

			sprintf(renderStageExternalInputAttachmentBuffer + 3, "%u", renderStageExternalInputAttachment.GlobalIndex);

			for (auto renderStageIt : renderStageExternalInputAttachment.RenderStages)
			{
				sprintf(renderStageBuffer + 2, "%u", renderStageIt.GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageExternalInputAttachmentBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, " [color=red];\n");
				fputs(fileOutputBuffer, pFile);
			}
		}

		//Output Attachments
		for (auto& attachmentPair : internalOutputAttachments)
		{
			const InternalRenderStageOutputAttachment& renderStageOutputAttachment = attachmentPair.second;

			sprintf(renderStageOutputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.GlobalIndex);

			for (auto renderStageIt : renderStageOutputAttachment.RenderStages)
			{
				sprintf(renderStageBuffer + 2, "%u", renderStageIt.GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
				strcat(fileOutputBuffer, ";\n");
				fputs(fileOutputBuffer, pFile);
			}

			if (renderStageOutputAttachment.pConnectedAttachment != nullptr)
			{
				sprintf(renderStageInputAttachmentBuffer + 2, "%u", renderStageOutputAttachment.pConnectedAttachment->GlobalIndex);

				strcpy(fileOutputBuffer, "\t");
				strcat(fileOutputBuffer, renderStageOutputAttachmentBuffer);
				strcat(fileOutputBuffer, " -> ");
				strcat(fileOutputBuffer, renderStageInputAttachmentBuffer);
				strcat(fileOutputBuffer, ";\n");
				fputs(fileOutputBuffer, pFile);
			}
		}
	}
}