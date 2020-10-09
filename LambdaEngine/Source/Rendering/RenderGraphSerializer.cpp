#include "PreCompiled.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderGraphParser.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

namespace LambdaEngine
{
	bool RenderGraphSerializer::SaveRenderGraphToFile(
		const String& renderGraphName, 
		const TArray<RenderGraphResourceDesc>& resources, 
		const THashTable<String, EditorRenderStageDesc>& renderStagesByName, 
		const THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex, 
		const THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex, 
		const TArray<EditorResourceStateGroup>& resourceStateGroups, 
		const EditorFinalOutput& finalOutput)
	{
		using namespace rapidjson;

		StringBuffer jsonStringBuffer;
		PrettyWriter<StringBuffer> writer(jsonStringBuffer);

		writer.StartObject();

		//Resources
		{
			writer.String("resources");
			writer.StartArray();
			{
				for (const RenderGraphResourceDesc& resource : resources)
				{
					writer.StartObject();
					{
						writer.String("name");
						writer.String(resource.Name.c_str());

						writer.String("type");
						writer.String(RenderGraphResourceTypeToString(resource.Type));

						writer.String("back_buffer_bound");
						writer.Bool(resource.BackBufferBound);

						writer.String("sub_resource_count");
						writer.Uint(resource.SubResourceCount);

						writer.String("editable");
						writer.Bool(resource.Editable);

						writer.String("external");
						writer.Bool(resource.External);

						writer.String("type_params");
						writer.StartObject();
						{
							switch (resource.Type)
							{
								case ERenderGraphResourceType::TEXTURE:
								{
									writer.String("texture_format");
									writer.String(TextureFormatToString(resource.TextureParams.TextureFormat));

									writer.String("is_of_array_type");
									writer.Bool(resource.TextureParams.IsOfArrayType);

									writer.String("unbounded_array");
									writer.Bool(resource.TextureParams.UnboundedArray);

									writer.String("texture_type");
									writer.String(ResourceTextureTypeToString(resource.TextureParams.TextureType));

									if (!resource.External && resource.Name != RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
									{
										writer.String("x_dim_type");
										writer.String(RenderGraphDimensionTypeToString(resource.TextureParams.XDimType));

										writer.String("y_dim_type");
										writer.String(RenderGraphDimensionTypeToString(resource.TextureParams.YDimType));

										writer.String("x_dim_var");
										writer.Double(resource.TextureParams.XDimVariable);

										writer.String("y_dim_var");
										writer.Double(resource.TextureParams.YDimVariable);

										writer.String("sample_count");
										writer.Int(resource.TextureParams.SampleCount);

										writer.String("miplevel_count");
										writer.Int(resource.TextureParams.MiplevelCount);

										writer.String("sampler_type");
										writer.String(RenderGraphSamplerTypeToString(resource.TextureParams.SamplerType));

										writer.String("sampler_address_mode");
										writer.String(RenderGraphSamplerAddressModeToString(resource.TextureParams.SamplerAddressMode));

										writer.String("sampler_border_color");
										writer.String(RenderGraphSamplerBorderColorToString(resource.TextureParams.SamplerBorderColor));

										writer.String("memory_type");
										writer.String(MemoryTypeToString(resource.MemoryType));
									}

									break;
								}
								case ERenderGraphResourceType::SCENE_DRAW_ARGS:
								{
									break;
								}
								case ERenderGraphResourceType::BUFFER:
								{
									if (!resource.External)
									{
										writer.String("size_type");
										writer.String(RenderGraphDimensionTypeToString(resource.BufferParams.SizeType));

										writer.String("size");
										writer.Int(resource.BufferParams.Size);

										writer.String("memory_type");
										writer.String(MemoryTypeToString(resource.MemoryType));
									}
									break;
								}
								case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
								{
									if (!resource.External)
									{

									}
									break;
								}
							}
						}
						writer.EndObject();
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		//Resource State Group
		{
			writer.String("resource_state_groups");
			writer.StartArray();
			{
				for (const EditorResourceStateGroup& resourceStateGroup : resourceStateGroups)
				{
					writer.StartObject();
					{
						writer.String("name");
						writer.String(resourceStateGroup.Name.c_str());

						writer.String("resource_states");
						writer.StartArray();
						{
							for (const EditorResourceStateIdent& resourceStateIdent : resourceStateGroup.ResourceStateIdents)
							{
								int32 attributeIndex = resourceStateIdent.AttributeIndex;
								auto resourceStateIt = resourceStatesByHalfAttributeIndex.find(attributeIndex / 2);

								if (resourceStateIt == resourceStatesByHalfAttributeIndex.end())
								{
									LOG_ERROR("[RenderGraphSerializer]: Resource State %s could not be found", resourceStateIdent.Name.c_str());
									return false;
								}

								writer.StartObject();
								{
									writer.String("name");
									writer.String(resourceStateIt->second.ResourceName.c_str());

									writer.String("removable");
									writer.Bool(resourceStateIt->second.Removable);

									writer.String("binding_type");
									writer.String(BindingTypeToString(resourceStateIt->second.BindingType));

									writer.String("src_stage");

									if (resourceStateIt->second.InputLinkIndex >= 0)
									{
										auto resourceLinkIt = resourceStateLinksByLinkIndex.find(resourceStateIt->second.InputLinkIndex);

										if (resourceLinkIt == resourceStateLinksByLinkIndex.end())
										{
											LOG_ERROR("[RenderGraphSerializer]: Resource State Input Link %s could not be found", resourceStateIdent.Name.c_str());
											return false;
										}

										auto srcResourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceLinkIt->second.SrcAttributeIndex / 2);

										if (srcResourceStateIt == resourceStatesByHalfAttributeIndex.end())
										{
											LOG_ERROR("[RenderGraphSerializer]: Src Resource State %s could not be found", resourceStateIdent.Name.c_str());
											return false;
										}

										writer.String(srcResourceStateIt->second.RenderStageName.c_str());
									}
									else
										writer.String("");
								}
								writer.EndObject();
							}
						}
						writer.EndArray();
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		//Final Output Stage
		{
			writer.String("final_output_stage");
			writer.StartObject();
			{
				writer.String("name");
				writer.String(finalOutput.Name.c_str());

				writer.String("back_buffer_state");
				writer.StartObject();
				{
					int32 attributeIndex = finalOutput.BackBufferAttributeIndex;
					auto resourceStateIt = resourceStatesByHalfAttributeIndex.find(attributeIndex / 2);

					if (resourceStateIt == resourceStatesByHalfAttributeIndex.end())
					{
						LOG_ERROR("[RenderGraphSerializer]: Resource State %s could not be found", finalOutput.Name.c_str());
						return false;
					}

					writer.String("name");
					writer.String(resourceStateIt->second.ResourceName.c_str());

					writer.String("removable");
					writer.Bool(resourceStateIt->second.Removable);

					writer.String("binding_type");
					writer.String(BindingTypeToString(resourceStateIt->second.BindingType));

					writer.String("src_stage");

					if (resourceStateIt->second.InputLinkIndex >= 0)
					{
						auto resourceLinkIt = resourceStateLinksByLinkIndex.find(resourceStateIt->second.InputLinkIndex);

						if (resourceLinkIt == resourceStateLinksByLinkIndex.end())
						{
							LOG_ERROR("[RenderGraphSerializer]: Resource State Input Link %s could not be found", finalOutput.Name.c_str());
							return false;
						}

						auto srcResourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceLinkIt->second.SrcAttributeIndex / 2);

						if (srcResourceStateIt == resourceStatesByHalfAttributeIndex.end())
						{
							LOG_ERROR("[RenderGraphSerializer]: Src Resource State %s could not be found", finalOutput.Name.c_str());
							return false;
						}

						writer.String(srcResourceStateIt->second.RenderStageName.c_str());
					}
					else
						writer.String("");
				}
				writer.EndObject();
			}
			writer.EndObject();
		}

		//Render Stages
		{
			writer.String("render_stages");
			writer.StartArray();
			{
				for (auto renderStageIt = renderStagesByName.begin(); renderStageIt != renderStagesByName.end(); renderStageIt++)
				{
					writer.StartObject();
					{
						writer.String("name");
						writer.String(renderStageIt->second.Name.c_str());

						writer.String("type");
						writer.String(RenderStageTypeToString(renderStageIt->second.Type));

						writer.String("custom_renderer");
						writer.Bool(renderStageIt->second.CustomRenderer);

						writer.String("allow_overriding_of_binding_types");
						writer.Bool(renderStageIt->second.OverrideRecommendedBindingType);

						writer.String("trigger_type");
						writer.String(ExecutionTriggerTypeToString(renderStageIt->second.TriggerType).c_str());

						writer.String("frame_delay");
						writer.Int(renderStageIt->second.FrameDelay);

						writer.String("frame_offset");
						writer.Int(renderStageIt->second.FrameOffset);

						writer.String("x_dim_type");
						writer.String(RenderGraphDimensionTypeToString(renderStageIt->second.Parameters.XDimType));

						writer.String("y_dim_type");
						writer.String(RenderGraphDimensionTypeToString(renderStageIt->second.Parameters.YDimType));

						writer.String("z_dim_type");
						writer.String(RenderGraphDimensionTypeToString(renderStageIt->second.Parameters.ZDimType));

						writer.String("x_dim_var");
						writer.Double(renderStageIt->second.Parameters.XDimVariable);

						writer.String("y_dim_var");
						writer.Double(renderStageIt->second.Parameters.YDimVariable);

						writer.String("z_dim_var");
						writer.Double(renderStageIt->second.Parameters.ZDimVariable);

						if (renderStageIt->second.Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
						{
							writer.String("draw_type");
							writer.String(RenderStageDrawTypeToString(renderStageIt->second.Graphics.DrawType));

							writer.String("depth_test_enabled");
							writer.Bool(renderStageIt->second.Graphics.DepthTestEnabled);

							writer.String("cull_mode");
							writer.String(CullModeToString(renderStageIt->second.Graphics.CullMode));

							writer.String("polygon_mode");
							writer.String(PolygonModeToString(renderStageIt->second.Graphics.PolygonMode));

							writer.String("primitive_topology");
							writer.String(PrimitiveTopologyToString(renderStageIt->second.Graphics.PrimitiveTopology));					
						}

						writer.String("shaders");
						writer.StartObject();
						{
							if (renderStageIt->second.Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
							{
								writer.String("task_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.TaskShaderName.c_str());

								writer.String("mesh_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.MeshShaderName.c_str());

								writer.String("vertex_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.VertexShaderName.c_str());

								writer.String("geometry_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.GeometryShaderName.c_str());

								writer.String("hull_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.HullShaderName.c_str());

								writer.String("domain_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.DomainShaderName.c_str());

								writer.String("pixel_shader");
								writer.String(renderStageIt->second.Graphics.Shaders.PixelShaderName.c_str());
							}
							else if (renderStageIt->second.Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
							{
								writer.String("shader");
								writer.String(renderStageIt->second.Compute.ShaderName.c_str());
							}
							else if (renderStageIt->second.Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
							{
								writer.String("raygen_shader");
								writer.String(renderStageIt->second.RayTracing.Shaders.RaygenShaderName.c_str());

								writer.String("miss_shaders");
								writer.StartArray();
								for (uint32 m = 0; m < renderStageIt->second.RayTracing.Shaders.MissShaderCount; m++)
								{
									writer.String(renderStageIt->second.RayTracing.Shaders.pMissShaderNames[m].c_str());
								}
								writer.EndArray();

								writer.String("closest_hit_shaders");
								writer.StartArray();
								for (uint32 ch = 0; ch < renderStageIt->second.RayTracing.Shaders.ClosestHitShaderCount; ch++)
								{
									writer.String(renderStageIt->second.RayTracing.Shaders.pClosestHitShaderNames[ch].c_str());
								}
								writer.EndArray();
							}
						}
						writer.EndObject();

						writer.String("resource_states");
						writer.StartArray();
						{
							for (const EditorResourceStateIdent& resourceStateIdent : renderStageIt->second.ResourceStateIdents)
							{
								int32 attributeIndex = resourceStateIdent.AttributeIndex;
								auto resourceStateIt = resourceStatesByHalfAttributeIndex.find(attributeIndex / 2);

								if (resourceStateIt == resourceStatesByHalfAttributeIndex.end())
								{
									LOG_ERROR("[RenderGraphSerializer]: Resource State %s could not be found", resourceStateIdent.Name.c_str());
									return false;
								}

								writer.StartObject();
								{
									writer.String("name");
									writer.String(resourceStateIt->second.ResourceName.c_str());

									writer.String("removable");
									writer.Bool(resourceStateIt->second.Removable);

									writer.String("draw_args_mask");
									writer.Uint(resourceStateIt->second.DrawArgsMask);

									writer.String("binding_type");
									writer.String(BindingTypeToString(resourceStateIt->second.BindingType));


									writer.String("src_stage");

									if (resourceStateIt->second.InputLinkIndex >= 0)
									{
										auto resourceLinkIt = resourceStateLinksByLinkIndex.find(resourceStateIt->second.InputLinkIndex);

										if (resourceLinkIt == resourceStateLinksByLinkIndex.end())
										{
											LOG_ERROR("[RenderGraphSerializer]: Resource State Input Link %s could not be found", finalOutput.Name.c_str());
											return false;
										}

										auto srcResourceStateIt = resourceStatesByHalfAttributeIndex.find(resourceLinkIt->second.SrcAttributeIndex / 2);

										if (srcResourceStateIt == resourceStatesByHalfAttributeIndex.end())
										{
											LOG_ERROR("[RenderGraphSerializer]: Src Resource State %s could not be found", finalOutput.Name.c_str());
											return false;
										}

										writer.String(srcResourceStateIt->second.RenderStageName.c_str());
									}
									else
										writer.String("");
								}
								writer.EndObject();
							}
						}
						writer.EndArray();
					}
					writer.EndObject();
				}
			}
			writer.EndArray();
		}

		writer.EndObject();

		FILE* pFile = fopen(("../Assets/RenderGraphs/" + renderGraphName + ".lrg").c_str(), "w");

		if (pFile != nullptr)
		{
			fputs(jsonStringBuffer.GetString(), pFile);
			fclose(pFile);
			return true;
		}
		
		return false;
	}

	bool RenderGraphSerializer::LoadFromFile(
		const String& renderGraphName, 
		TArray<RenderGraphResourceDesc>& resources, 
		THashTable<int32, String>& renderStageNameByInputAttributeIndex, 
		THashTable<String, EditorRenderStageDesc>& renderStagesByName, 
		THashTable<int32, EditorRenderGraphResourceState>& resourceStatesByHalfAttributeIndex, 
		THashTable<int32, EditorRenderGraphResourceLink>& resourceStateLinksByLinkIndex, 
		TArray<EditorResourceStateGroup>& resourceStateGroups, 
		EditorFinalOutput& finalOutput,
		int32& nextNodeID,
		int32& nextAttributeID,
		int32& nextLinkID)
	{
		using namespace rapidjson;

		if (renderGraphName.empty())
		{
			LOG_ERROR("[RenderGraphSerializer]: RenderGraph name can not be empty");
			return false;
		}

		//Reset to Clear state
		{
			nextNodeID		= 0;
			nextAttributeID = 0;
			nextLinkID		= 0;

			resources.Clear();
			renderStageNameByInputAttributeIndex.clear();
			renderStagesByName.clear();
			resourceStatesByHalfAttributeIndex.clear();
			resourceStateLinksByLinkIndex.clear();

			resourceStateGroups.Clear();
			finalOutput = {};
		}

		THashTable<String, TArray<int32>>					unfinishedLinks; //The key is the Resource State Group / Render Stage name, the value is the resource states awaiting linking

		String filepath = "../Assets/RenderGraphs/" + renderGraphName;
		
		FILE* pFile = fopen(filepath.c_str(), "r");

		if (pFile == nullptr)
		{
			LOG_ERROR("[RenderGraphSerializer]: Failed to open RenderGraph file with path %s", filepath.c_str());
			return false;
		}

		constexpr uint32 READ_BUFFER_SIZE = 65536;
		char* pReadBuffer = DBG_NEW char[READ_BUFFER_SIZE]; //We allocate this on the heap, otherwise Visual Studio gives warning because its large
		FileReadStream inputStream(pFile, pReadBuffer, READ_BUFFER_SIZE);

		Document d;
		d.ParseStream(inputStream);

		//Load Resources
		if (d.HasMember("resources"))
		{
			if (d["resources"].IsArray())
			{
				GenericArray resourceArray = d["resources"].GetArray();

				for (uint32 r = 0; r < resourceArray.Size(); r++)
				{
					GenericObject resourceObject = resourceArray[r].GetObject();
					RenderGraphResourceDesc resource = {};
					resource.Name							= resourceObject["name"].GetString();
					resource.Type							= RenderGraphResourceTypeFromString(resourceObject["type"].GetString());
					resource.BackBufferBound				= resourceObject.HasMember("back_buffer_bound") ? resourceObject["back_buffer_bound"].GetBool() : false;
					resource.SubResourceCount				= resourceObject["sub_resource_count"].GetUint();
					
					resource.Editable						= resourceObject["editable"].GetBool();
					resource.External						= resourceObject["external"].GetBool();

					GenericObject resourceTypeParamsObject = resourceObject["type_params"].GetObject();
					{
						switch (resource.Type)
						{
							case ERenderGraphResourceType::TEXTURE:
							{
								resource.TextureParams.TextureFormat																= TextureFormatFromString(resourceTypeParamsObject["texture_format"].GetString());
								resource.TextureParams.IsOfArrayType																= resourceTypeParamsObject["is_of_array_type"].GetBool();
								if (resourceTypeParamsObject.HasMember("unbounded_array")) resource.TextureParams.UnboundedArray	= resourceTypeParamsObject["unbounded_array"].GetBool();
								if (resourceTypeParamsObject.HasMember("texture_type")) resource.TextureParams.TextureType			= ResourceTextureTypeFromString(resourceTypeParamsObject["texture_type"].GetString());

								if (!resource.External && resource.Name != RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
								{
									resource.TextureParams.XDimType				= RenderGraphDimensionTypeFromString(resourceTypeParamsObject["x_dim_type"].GetString());
									resource.TextureParams.YDimType				= RenderGraphDimensionTypeFromString(resourceTypeParamsObject["y_dim_type"].GetString());
									resource.TextureParams.XDimVariable			= resourceTypeParamsObject["x_dim_var"].GetFloat();
									resource.TextureParams.YDimVariable			= resourceTypeParamsObject["y_dim_var"].GetFloat();
									resource.TextureParams.SampleCount			= resourceTypeParamsObject["sample_count"].GetInt();
									resource.TextureParams.MiplevelCount		= resourceTypeParamsObject["miplevel_count"].GetInt();
									resource.TextureParams.SamplerType			= RenderGraphSamplerTypeFromString(resourceTypeParamsObject["sampler_type"].GetString());
									resource.TextureParams.SamplerAddressMode	= resourceTypeParamsObject.HasMember("sampler_address_mode") ? RenderGraphSamplerAddressModeFromString(resourceTypeParamsObject["sampler_address_mode"].GetString()) : ERenderGraphSamplerAddressMode::REPEAT;
									resource.TextureParams.SamplerBorderColor	= resourceTypeParamsObject.HasMember("sampler_border_color") ? RenderGraphSamplerBorderColorFromString(resourceTypeParamsObject["sampler_border_color"].GetString()) : ERenderGraphSamplerBorderColor::BORDER_COLOR_FLOAT_OPAQUE_BLACK;
									resource.MemoryType							= MemoryTypeFromString(resourceTypeParamsObject["memory_type"].GetString());
								}
								break;
							}
							case ERenderGraphResourceType::BUFFER:
							{
								if (!resource.External)
								{
									resource.BufferParams.SizeType			= RenderGraphDimensionTypeFromString(resourceTypeParamsObject["size_type"].GetString());
									resource.BufferParams.Size				= resourceTypeParamsObject["size"].GetInt();
									resource.MemoryType						= MemoryTypeFromString(resourceTypeParamsObject["memory_type"].GetString());
								}
								break;
							}
							case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
							{
								break;
							}
						}
					}
					
					resources.PushBack(resource);
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"resources\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"resources\" member could not be found!");
			return false;
		}

		//Load Resource State Groups
		if (d.HasMember("resource_state_groups"))
		{
			if (d["resource_state_groups"].IsArray())
			{
				GenericArray resourceStateGroupsArray = d["resource_state_groups"].GetArray();

				for (uint32 rsg = 0; rsg < resourceStateGroupsArray.Size(); rsg++)
				{
					GenericObject resourceStateGroupObject = resourceStateGroupsArray[rsg].GetObject();
					EditorResourceStateGroup resourceStateGroup = {};
					resourceStateGroup.Name				= resourceStateGroupObject["name"].GetString();
					resourceStateGroup.InputNodeIndex	= nextNodeID++;
					resourceStateGroup.OutputNodeIndex	= nextNodeID++;

					auto unfinishedLinkIt = unfinishedLinks.find(resourceStateGroup.Name);

					GenericArray resourceStateArray = resourceStateGroupObject["resource_states"].GetArray();

					for (uint32 r = 0; r < resourceStateArray.Size(); r++)
					{
						GenericObject resourceStateObject = resourceStateArray[r].GetObject();

						String resourceName		= resourceStateObject["name"].GetString();

						int32 attributeIndex = nextAttributeID;
						nextAttributeID += 2;

						EditorRenderGraphResourceState* pResourceState = &resourceStatesByHalfAttributeIndex[attributeIndex / 2];
						pResourceState->ResourceName		= resourceName;
						pResourceState->RenderStageName		= resourceStateGroup.Name;
						pResourceState->Removable			= resourceStateObject["removable"].GetBool();
						pResourceState->BindingType			= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());

						auto resourceIt = std::find_if(resources.Begin(), resources.End(), [pResourceState](const RenderGraphResourceDesc& resourceDesc) { return pResourceState->ResourceName == resourceDesc.Name; });
						if (resourceIt == resources.End())
						{
							LOG_ERROR("[RenderGraphSerializer]: Resource State %s was not found in Resources Array", pResourceState->ResourceName.c_str());
							return false;
						}

						pResourceState->ResourceType		= resourceIt->Type;

						resourceStateGroup.ResourceStateIdents.PushBack({ resourceName, attributeIndex });

						//Check if there are resource states that are awaiting linking to this resource state group
						if (unfinishedLinkIt != unfinishedLinks.end())
						{
							if (FixLinkForPreviouslyLoadedResourceState(
								pResourceState,
								attributeIndex,
								resourceStatesByHalfAttributeIndex,
								resourceStateLinksByLinkIndex,
								unfinishedLinkIt->second,
								nextLinkID))
							{
								if (unfinishedLinkIt->second.IsEmpty())
								{
									unfinishedLinks.erase(unfinishedLinkIt);
									unfinishedLinkIt = unfinishedLinks.end();
								}
							}
						}

						//Load Src Stage and check if we can link to it, otherwise we need to add this resource state to unfinishedLinks
						{
							String srcStageName = resourceStateObject["src_stage"].GetString();

							CreateLinkForLoadedResourceState(
								pResourceState,
								attributeIndex,
								srcStageName,
								resourceStateGroups,
								renderStagesByName,
								resourceStatesByHalfAttributeIndex,
								resourceStateLinksByLinkIndex,
								unfinishedLinks,
								nextLinkID);
						}
					}

					resourceStateGroups.PushBack(resourceStateGroup);
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member could not be found!");
			return false;
		}

		//Load Final Output Stage
		if (d.HasMember("final_output_stage"))
		{
			if (d["final_output_stage"].IsObject())
			{
				GenericObject finalOutputStageObject = d["final_output_stage"].GetObject();

				finalOutput.Name		= finalOutputStageObject["name"].GetString();
				finalOutput.NodeIndex	= nextNodeID++;

				GenericObject resourceStateObject = finalOutputStageObject["back_buffer_state"].GetObject();

				String resourceName		= resourceStateObject["name"].GetString();

				int32 attributeIndex = nextAttributeID;
				nextAttributeID += 2;

				EditorRenderGraphResourceState* pResourceState = &resourceStatesByHalfAttributeIndex[attributeIndex / 2];
				pResourceState->ResourceName		= resourceName;
				pResourceState->RenderStageName		= finalOutput.Name;
				pResourceState->Removable			= resourceStateObject["removable"].GetBool();
				pResourceState->BindingType			= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());

				auto resourceIt = std::find_if(resources.Begin(), resources.End(), [pResourceState](const RenderGraphResourceDesc& resourceDesc) { return pResourceState->ResourceName == resourceDesc.Name; });
				if (resourceIt == resources.End())
				{
					LOG_ERROR("[RenderGraphSerializer]: Resource State % in Final Output was not found in Resources Array", pResourceState->ResourceName.c_str());
					return false;
				}

				pResourceState->ResourceType = resourceIt->Type;

				finalOutput.BackBufferAttributeIndex = attributeIndex;

				//Load Src Stage and check if we can link to it, otherwise we need to add this resource state to unfinishedLinks
				{
					String srcStageName = resourceStateObject["src_stage"].GetString();

					CreateLinkForLoadedResourceState(
						pResourceState,
						attributeIndex,
						srcStageName,
						resourceStateGroups,
						renderStagesByName,
						resourceStatesByHalfAttributeIndex,
						resourceStateLinksByLinkIndex,
						unfinishedLinks,
						nextLinkID);
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"external_resources_stage\" member could not be found!");
			return false;
		}

		//Load Render Stages and Render Stage Resource States
		if (d.HasMember("render_stages"))
		{
			if (d["render_stages"].IsArray())
			{
				GenericArray renderStageArray = d["render_stages"].GetArray();

				for (uint32 rs = 0; rs < renderStageArray.Size(); rs++)
				{
					GenericObject renderStageObject = renderStageArray[rs].GetObject();
					EditorRenderStageDesc renderStage = {};
					renderStage.Name				= renderStageObject["name"].GetString();
					renderStage.NodeIndex			= nextNodeID++;
					renderStage.InputAttributeIndex = nextAttributeID;
					nextAttributeID					+= 2;
					renderStage.Type				= RenderStageTypeFromString(renderStageObject["type"].GetString());
					renderStage.CustomRenderer		= renderStageObject["custom_renderer"].GetBool();
					renderStage.OverrideRecommendedBindingType = renderStageObject.HasMember("allow_overriding_of_binding_types") ? renderStageObject["allow_overriding_of_binding_types"].GetBool() : false;
					
					renderStage.TriggerType = renderStageObject.HasMember("trigger_type")	? ExecutionTriggerTypeFromString(renderStageObject["trigger_type"].GetString()) : ERenderStageExecutionTrigger::EVERY;
					renderStage.FrameDelay	= renderStageObject.HasMember("frame_delay")	? renderStageObject["frame_delay"].GetInt()		: 0;
					renderStage.FrameOffset = renderStageObject.HasMember("frame_offset")	? renderStageObject["frame_offset"].GetInt()	: 0;

					renderStage.Parameters.XDimType			= RenderGraphDimensionTypeFromString(renderStageObject["x_dim_type"].GetString());
					renderStage.Parameters.YDimType			= RenderGraphDimensionTypeFromString(renderStageObject["y_dim_type"].GetString());
					renderStage.Parameters.ZDimType			= RenderGraphDimensionTypeFromString(renderStageObject["z_dim_type"].GetString());

					renderStage.Parameters.XDimVariable		= renderStageObject["x_dim_var"].GetFloat();
					renderStage.Parameters.YDimVariable		= renderStageObject["y_dim_var"].GetFloat();
					renderStage.Parameters.ZDimVariable		= renderStageObject["z_dim_var"].GetFloat();

					auto unfinishedLinkIt = unfinishedLinks.find(renderStage.Name);

					GenericObject shadersObject		= renderStageObject["shaders"].GetObject();
					GenericArray resourceStateArray = renderStageObject["resource_states"].GetArray();

					if (renderStage.Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						renderStage.Graphics.DrawType = RenderStageDrawTypeFromString(renderStageObject["draw_type"].GetString());

						renderStage.Graphics.DepthTestEnabled					= renderStageObject["depth_test_enabled"].GetBool();
						if (renderStageObject.HasMember("cull_mode"))			renderStage.Graphics.CullMode			= CullModeFromString(renderStageObject["cull_mode"].GetString());
						if (renderStageObject.HasMember("polygon_mode"))		renderStage.Graphics.PolygonMode		= PolygonModeFromString(renderStageObject["polygon_mode"].GetString());
						if (renderStageObject.HasMember("primitive_topology"))	renderStage.Graphics.PrimitiveTopology	= PrimitiveTopologyFromString(renderStageObject["primitive_topology"].GetString());

						renderStage.Graphics.Shaders.TaskShaderName		= shadersObject["task_shader"].GetString();
						renderStage.Graphics.Shaders.MeshShaderName		= shadersObject["mesh_shader"].GetString();
						renderStage.Graphics.Shaders.VertexShaderName	= shadersObject["vertex_shader"].GetString();
						renderStage.Graphics.Shaders.GeometryShaderName	= shadersObject["geometry_shader"].GetString();
						renderStage.Graphics.Shaders.HullShaderName		= shadersObject["hull_shader"].GetString();
						renderStage.Graphics.Shaders.DomainShaderName	= shadersObject["domain_shader"].GetString();
						renderStage.Graphics.Shaders.PixelShaderName	= shadersObject["pixel_shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
					{
						renderStage.Compute.ShaderName			= shadersObject["shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
					{
						renderStage.RayTracing.Shaders.RaygenShaderName	= shadersObject["raygen_shader"].GetString();

						GenericArray missShadersArray = shadersObject["miss_shaders"].GetArray();

						for (uint32 m = 0; m < missShadersArray.Size(); m++)
						{
							renderStage.RayTracing.Shaders.pMissShaderNames[m] = missShadersArray[m].GetString();
						}

						renderStage.RayTracing.Shaders.MissShaderCount = missShadersArray.Size();

						GenericArray closestHitShadersArray = shadersObject["closest_hit_shaders"].GetArray();

						for (uint32 ch = 0; ch < closestHitShadersArray.Size(); ch++)
						{
							renderStage.RayTracing.Shaders.pClosestHitShaderNames[ch] = closestHitShadersArray[ch].GetString();
						}

						renderStage.RayTracing.Shaders.ClosestHitShaderCount = closestHitShadersArray.Size();
					}

					for (uint32 r = 0; r < resourceStateArray.Size(); r++)
					{
						GenericObject resourceStateObject = resourceStateArray[r].GetObject();

						String resourceName		= resourceStateObject["name"].GetString();

						int32 attributeIndex = nextAttributeID;
						nextAttributeID += 2;

						EditorRenderGraphResourceState* pResourceState = &resourceStatesByHalfAttributeIndex[attributeIndex / 2];
						pResourceState->ResourceName		= resourceName;
						pResourceState->RenderStageName		= renderStage.Name;
						pResourceState->Removable			= resourceStateObject["removable"].GetBool();
						if (resourceStateObject.HasMember("draw_args_mask"))	pResourceState->DrawArgsMask = resourceStateObject["draw_args_mask"].GetUint();
						pResourceState->BindingType			= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());

						auto resourceIt = std::find_if(resources.Begin(), resources.End(), [pResourceState](const RenderGraphResourceDesc& resourceDesc) { return pResourceState->ResourceName == resourceDesc.Name; });
						if (resourceIt == resources.End())
						{
							LOG_ERROR("[RenderGraphSerializer]: Resource State %s in Render Stage %s was not found in Resources Array", pResourceState->ResourceName.c_str(), renderStage.Name.c_str());
							return false;
						}

						pResourceState->ResourceType = resourceIt->Type;

						renderStage.ResourceStateIdents.PushBack({ resourceName, attributeIndex });

						//Check if there are resource states that are awaiting linking to this resource state group
						if (unfinishedLinkIt != unfinishedLinks.end())
						{
							if (FixLinkForPreviouslyLoadedResourceState(
								pResourceState,
								attributeIndex,
								resourceStatesByHalfAttributeIndex,
								resourceStateLinksByLinkIndex,
								unfinishedLinkIt->second,
								nextLinkID))
							{
								if (unfinishedLinkIt->second.IsEmpty())
								{
									unfinishedLinks.erase(unfinishedLinkIt);
									unfinishedLinkIt = unfinishedLinks.end();
								}
							}
						}

						//Load Src Stage and check if we can link to it, otherwise we need to add this resource state to unfinishedLinks
						{
							String srcStageName = resourceStateObject["src_stage"].GetString();

							CreateLinkForLoadedResourceState(
								pResourceState,
								attributeIndex,
								srcStageName,
								resourceStateGroups,
								renderStagesByName,
								resourceStatesByHalfAttributeIndex,
								resourceStateLinksByLinkIndex,
								unfinishedLinks,
								nextLinkID);
						}
					}

					renderStagesByName[renderStage.Name] = renderStage;
					renderStageNameByInputAttributeIndex[renderStage.InputAttributeIndex] = renderStage.Name;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"render_stages\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"render_stages\" member could not be found!");
			return false;
		}

		fclose(pFile);

		if (!unfinishedLinks.empty())
		{
			LOG_ERROR("[RenderGraphEditor]: The following Resource States did not successfully link:");

			for (auto unfinishedLinkIt = unfinishedLinks.begin(); unfinishedLinkIt != unfinishedLinks.end(); unfinishedLinkIt++)
			{
				LOG_ERROR("\t--%s--", unfinishedLinkIt->first.c_str());

				for (int32 attributeIndex : unfinishedLinkIt->second)
				{
					EditorRenderGraphResourceState* pResourceState = &resourceStatesByHalfAttributeIndex[attributeIndex / 2];
					LOG_ERROR("\t\t%s", pResourceState->ResourceName.c_str());
				}
			}

			return false;
		}

		SAFEDELETE_ARRAY(pReadBuffer);
		return true;
	}

	bool RenderGraphSerializer::LoadAndParse(RenderGraphStructureDesc* pRenderGraphStructureDesc, const String& renderGraphName, bool imGuiEnabled)
	{
		TArray<RenderGraphResourceDesc> resources;
		THashTable<int32, String> renderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStageDesc> renderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState> resourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink> resourceStateLinksByLinkIndex;
		TArray<EditorResourceStateGroup> resourceStateGroups;
		EditorFinalOutput finalOutput;
		int32 nextNodeID;
		int32 nextAttributeID;
		int32 nextLinkID;

		if (!renderGraphName.empty())
		{
			if (!LoadFromFile(
				renderGraphName,
				resources,
				renderStageNameByInputAttributeIndex,
				renderStagesByName,
				resourceStatesByHalfAttributeIndex,
				resourceStateLinksByLinkIndex,
				resourceStateGroups,
				finalOutput,
				nextNodeID,
				nextAttributeID,
				nextLinkID))
			{
				LOG_ERROR("[RenderGraphSerializer]: Failed to load RenderGraph %s from file", renderGraphName.c_str());
				return false;
			}
		}

		if (!RenderGraphParser::ParseRenderGraph(
			pRenderGraphStructureDesc,
			resources,
			renderStagesByName,
			resourceStatesByHalfAttributeIndex,
			resourceStateLinksByLinkIndex,
			finalOutput,
			imGuiEnabled))
		{
			LOG_ERROR("[RenderGraphSerializer]: Failed to parse RenderGraph %s", renderGraphName.c_str());
			return false;
		}

		return true;
	}

	bool RenderGraphSerializer::FixLinkForPreviouslyLoadedResourceState(
		EditorRenderGraphResourceState* pResourceState,
		int32 attributeIndex,
		THashTable<int32, EditorRenderGraphResourceState>& loadedResourceStatesByHalfAttributeIndex, 
		THashTable<int32, EditorRenderGraphResourceLink>& loadedResourceStateLinks, 
		TArray<int32>& unfinishedLinksAwaitingStage,
		int32& nextLinkID)
	{
		bool linkedAtLeastOne = false;

		for (auto linkAwaitingResourceStateIt = unfinishedLinksAwaitingStage.begin(); linkAwaitingResourceStateIt != unfinishedLinksAwaitingStage.end();)
		{
			EditorRenderGraphResourceState* pLinkAwaitingResourceState = &loadedResourceStatesByHalfAttributeIndex[*linkAwaitingResourceStateIt / 2];

			if (pLinkAwaitingResourceState->ResourceName == pResourceState->ResourceName)
			{
				//Create Link
				EditorRenderGraphResourceLink* pLink = &loadedResourceStateLinks[nextLinkID];
				pLink->SrcAttributeIndex	= attributeIndex + 1;
				pLink->DstAttributeIndex	= (*linkAwaitingResourceStateIt);
				pLink->LinkIndex			= nextLinkID;
				nextLinkID++;

				//Update Current Resource State
				pResourceState->OutputLinkIndices.insert(pLink->LinkIndex);

				//Update Awaiting Resource State
				pLinkAwaitingResourceState->InputLinkIndex = pLink->LinkIndex;

				linkAwaitingResourceStateIt = unfinishedLinksAwaitingStage.Erase(linkAwaitingResourceStateIt);
				linkedAtLeastOne = true;
			}
			else
			{
				linkAwaitingResourceStateIt++;
			}
		}

		return linkedAtLeastOne;
	}

	void RenderGraphSerializer::CreateLinkForLoadedResourceState(
		EditorRenderGraphResourceState* pResourceState,
		int32 attributeIndex,
		String& srcStageName,
		TArray<EditorResourceStateGroup>& loadedResourceStateGroups,
		THashTable<String, EditorRenderStageDesc>& loadedRenderStagesByName,
		THashTable<int32, EditorRenderGraphResourceState>& loadedResourceStatesByHalfAttributeIndex,
		THashTable<int32, EditorRenderGraphResourceLink>& loadedResourceStateLinks,
		THashTable<String, TArray<int32>>& unfinishedLinks,
		int32& nextLinkID)
	{
		if (!srcStageName.empty())
		{
			auto renderStageIt			= loadedRenderStagesByName.find(srcStageName);
			bool foundSrcStage			= false;

			if (renderStageIt != loadedRenderStagesByName.end())
			{
				EditorRenderStageDesc* pRenderStageDesc = &renderStageIt->second;

				auto srcResourceStateIdentIt = pRenderStageDesc->FindResourceStateIdent(pResourceState->ResourceName);

				if (srcResourceStateIdentIt != pRenderStageDesc->ResourceStateIdents.end())
				{
					EditorRenderGraphResourceState* pSrcResourceStat = &loadedResourceStatesByHalfAttributeIndex[srcResourceStateIdentIt->AttributeIndex / 2];

					//Create Link
					EditorRenderGraphResourceLink* pLink = &loadedResourceStateLinks[nextLinkID];
					pLink->SrcAttributeIndex	= srcResourceStateIdentIt->AttributeIndex + 1;
					pLink->DstAttributeIndex	= attributeIndex;
					pLink->LinkIndex			= nextLinkID;
					nextLinkID++;

					//Update Current Resource State
					pResourceState->InputLinkIndex = pLink->LinkIndex;

					//Update Awaiting Resource State
					pSrcResourceStat->OutputLinkIndices.insert(pLink->LinkIndex);

					foundSrcStage = true;
				}
			}
			else
			{
				//Loop through resource state groups and check them
				for (EditorResourceStateGroup& resourceStateGroup : loadedResourceStateGroups)
				{
					if (resourceStateGroup.Name == srcStageName)
					{
						auto srcResourceStateIdentIt = resourceStateGroup.FindResourceStateIdent(pResourceState->ResourceName);

						if (srcResourceStateIdentIt != resourceStateGroup.ResourceStateIdents.end())
						{
							EditorRenderGraphResourceState* pSrcResourceStat = &loadedResourceStatesByHalfAttributeIndex[srcResourceStateIdentIt->AttributeIndex / 2];

							//Create Link
							EditorRenderGraphResourceLink* pLink = &loadedResourceStateLinks[nextLinkID];
							pLink->SrcAttributeIndex	= srcResourceStateIdentIt->AttributeIndex + 1;
							pLink->DstAttributeIndex	= attributeIndex;
							pLink->LinkIndex			= nextLinkID;
							nextLinkID++;

							//Update Current Resource State
							pResourceState->InputLinkIndex = pLink->LinkIndex;

							//Update Awaiting Resource State
							pSrcResourceStat->OutputLinkIndices.insert(pLink->LinkIndex);

							foundSrcStage = true;
						}
					}
				}

				//Final output cant be used as a source stage
			}

			//Add this resource state to unfinishedLinks
			if (!foundSrcStage)
			{
				unfinishedLinks[srcStageName].PushBack(attributeIndex);
			}
		}
	}
}