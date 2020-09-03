#include "Rendering/RenderGraphEditor.h"

#include "Application/API/CommonApplication.h"

#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Log/Log.h"

#include "Utilities/IOUtilities.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderSystem.h"

namespace LambdaEngine
{
	int32		RenderGraphEditor::s_NextNodeID				= 0;
	int32		RenderGraphEditor::s_NextAttributeID		= 0;
	int32		RenderGraphEditor::s_NextLinkID				= 0;

	constexpr const uint32 HOVERED_COLOR	= IM_COL32(232, 27, 86, 255);
	constexpr const uint32 SELECTED_COLOR	= IM_COL32(162, 19, 60, 255);

	constexpr const uint32 EXTERNAL_RESOURCE_STATE_GROUP_INDEX	= 0;
	constexpr const uint32 TEMPORAL_RESOURCE_STATE_GROUP_INDEX	= 1;
	constexpr const uint32 NUM_RESOURCE_STATE_GROUPS			= 2;

	constexpr const int32 MAX_RESOURCE_NAME_LENGTH				= 256;

	constexpr const char* TEXTURE_FORMAT_NAMES[] =
	{
		"R32G32_SFLOAT",
		"R8G8B8A8_UNORM",
		"B8G8R8A8_UNORM",
		"R8G8B8A8_SNORM",
		"R16G16B16A16_SFLOAT",
		"D24_UNORM_S8_UINT",
		"R16_UNORM",
		"R32G32B32A32_SFLOAT",
		"R16_SFLOAT",
		"R32G32B32A32_UINT",
	};

	EFormat TextureFormatIndexToFormat(int32 index)
	{
		switch (index)
		{
			case 0: return EFormat::FORMAT_R32G32_SFLOAT;
			case 1: return EFormat::FORMAT_R8G8B8A8_UNORM;
			case 2: return EFormat::FORMAT_B8G8R8A8_UNORM;
			case 3: return EFormat::FORMAT_R8G8B8A8_SNORM;
			case 4: return EFormat::FORMAT_R16G16B16A16_SFLOAT;
			case 5: return EFormat::FORMAT_D24_UNORM_S8_UINT;
			case 6: return EFormat::FORMAT_R16_UNORM;
			case 7: return EFormat::FORMAT_R32G32B32A32_SFLOAT;
			case 8: return EFormat::FORMAT_R16_SFLOAT;
			case 9: return EFormat::FORMAT_R32G32B32A32_UINT;
		}

		return EFormat::NONE;
	}

	int32 TextureFormatToFormatIndex(EFormat format)
	{
		switch (format)
		{
			case EFormat::FORMAT_R32G32_SFLOAT:			return 0;
			case EFormat::FORMAT_R8G8B8A8_UNORM:		return 1;
			case EFormat::FORMAT_B8G8R8A8_UNORM:		return 2;
			case EFormat::FORMAT_R8G8B8A8_SNORM:		return 3;
			case EFormat::FORMAT_R16G16B16A16_SFLOAT:	return 4;
			case EFormat::FORMAT_D24_UNORM_S8_UINT:		return 5;
			case EFormat::FORMAT_R16_UNORM:				return 6;
			case EFormat::FORMAT_R32G32B32A32_SFLOAT:	return 7;
			case EFormat::FORMAT_R16_SFLOAT:			return 8;
			case EFormat::FORMAT_R32G32B32A32_UINT:		return 9;
		}

		return -1;
	}

	constexpr const char* DIMENSION_NAMES[] =
	{
		"CONSTANT",
		"EXTERNAL",
		"RELATIVE",
		"RELATIVE_1D",
	};

	ERenderGraphDimensionType DimensionTypeIndexToDimensionType(int32 index)
	{
		switch (index)
		{
			case 0: return ERenderGraphDimensionType::CONSTANT;
			case 1: return ERenderGraphDimensionType::EXTERNAL;
			case 2: return ERenderGraphDimensionType::RELATIVE;
			case 3: return ERenderGraphDimensionType::RELATIVE_1D;
		}

		return ERenderGraphDimensionType::NONE;
	}

	int32 DimensionTypeToDimensionTypeIndex(ERenderGraphDimensionType dimensionType)
	{
		switch (dimensionType)
		{
		case ERenderGraphDimensionType::CONSTANT:		return 0;
		case ERenderGraphDimensionType::EXTERNAL:		return 1;
		case ERenderGraphDimensionType::RELATIVE:		return 2;
		case ERenderGraphDimensionType::RELATIVE_1D:	return 3;
		}

		return -1;
	}

	constexpr const char* SAMPLER_NAMES[] =
	{
		"LINEAR",
		"NEAREST",
	};

	ERenderGraphSamplerType SamplerTypeIndexToSamplerType(int32 index)
	{
		switch (index)
		{
		case 0: return ERenderGraphSamplerType::LINEAR;
		case 1: return ERenderGraphSamplerType::NEAREST;
		}

		return ERenderGraphSamplerType::NONE;
	}

	int32 SamplerTypeToSamplerTypeIndex(ERenderGraphSamplerType samplerType)
	{
		switch (samplerType)
		{
		case ERenderGraphSamplerType::LINEAR:	return 0;
		case ERenderGraphSamplerType::NEAREST:	return 1;
		}

		return -1;
	}

	constexpr const char* MEMORY_TYPE_NAMES[] =
	{
		"MEMORY_CPU_VISIBLE",
		"MEMORY_GPU",
	};

	EMemoryType MemoryTypeIndexToMemoryType(int32 index)
	{
		switch (index)
		{
		case 0: return EMemoryType::MEMORY_CPU_VISIBLE;
		case 1: return EMemoryType::MEMORY_GPU;
		}

		return EMemoryType::NONE;
	}

	int32 MemoryTypeToMemoryTypeIndex(EMemoryType memoryType)
	{
		switch (memoryType)
		{
		case EMemoryType::MEMORY_CPU_VISIBLE:	return 0;
		case EMemoryType::MEMORY_GPU:			return 1;
		}

		return -1;
	}

	RenderGraphEditor::RenderGraphEditor()
	{
		CommonApplication::Get()->AddEventHandler(this);

		InitDefaultResources();
	}

	RenderGraphEditor::~RenderGraphEditor()
	{
	}

	void RenderGraphEditor::InitGUI()
	{
		imnodes::StyleColorsDark();

		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, HOVERED_COLOR);
		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, SELECTED_COLOR);

		imnodes::PushColorStyle(imnodes::ColorStyle_LinkHovered, HOVERED_COLOR);
		imnodes::PushColorStyle(imnodes::ColorStyle_LinkSelected, SELECTED_COLOR);

		m_GUIInitialized = true;

		SetInitialNodePositions();
	}

	void RenderGraphEditor::RenderGUI()
	{
		if (ImGui::Begin("Render Graph Editor"))
		{
			ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
			ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

			float contentRegionWidth	= contentRegionMax.x - contentRegionMin.x;
			float contentRegionHeight	= contentRegionMax.y - contentRegionMin.y;

			float maxResourcesViewTextWidth = 0.0f;
			float textHeight = ImGui::CalcTextSize("I").y + 5.0f;

			for (const RenderGraphResourceDesc& resource : m_Resources)
			{
				ImVec2 textSize = ImGui::CalcTextSize(resource.Name.c_str());

				maxResourcesViewTextWidth = textSize.x > maxResourcesViewTextWidth ? textSize.x : maxResourcesViewTextWidth;
			}

			for (auto shaderIt = m_FilesInShaderDirectory.begin(); shaderIt != m_FilesInShaderDirectory.end(); shaderIt++)
			{
				const String& shaderName = *shaderIt;
				ImVec2 textSize = ImGui::CalcTextSize(shaderName.c_str());

				maxResourcesViewTextWidth = textSize.x > maxResourcesViewTextWidth ? textSize.x : maxResourcesViewTextWidth;
			}

			float editButtonWidth = 120.0f;
			float removeButtonWidth = 120.0f;
			ImVec2 resourceViewSize(2.0f * maxResourcesViewTextWidth + editButtonWidth + removeButtonWidth, contentRegionHeight);

			if (ImGui::BeginChild("##Graph Resources View", resourceViewSize))
			{
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Resources");
				ImGui::NewLine();

				if (ImGui::BeginChild("##Resource View", ImVec2(0.0f, contentRegionHeight * 0.5f - textHeight * 5.0f), true, ImGuiWindowFlags_MenuBar))
				{
					RenderResourceView(maxResourcesViewTextWidth, textHeight);
					RenderAddResourceView();
					RenderEditResourceView();
				}
				ImGui::EndChild();

				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shaders");
				ImGui::NewLine();

				if (ImGui::BeginChild("##Shader View", ImVec2(0.0f, contentRegionHeight * 0.5f - textHeight * 5.0f), true))
				{
					RenderShaderView(maxResourcesViewTextWidth, textHeight);
				}
				ImGui::EndChild();

				if (ImGui::Button("Refresh Shaders"))
				{
					m_FilesInShaderDirectory = EnumerateFilesInDirectory("../Assets/Shaders/", true);
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("##Graph Views", ImVec2(contentRegionWidth - resourceViewSize.x, 0.0f)))
			{
				if (ImGui::BeginTabBar("##Render Graph Editor Tabs"))
				{
					if (ImGui::BeginTabItem("Graph Editor"))
					{
						if (ImGui::BeginChild("##Graph Editor View", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_MenuBar))
						{
							RenderGraphView();
							RenderAddRenderStageView();
							RenderSaveRenderGraphView();
							RenderLoadRenderGraphView();
						}

						ImGui::EndChild();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Parsed Graph"))
					{
						if (ImGui::BeginChild("##Parsed Graph View"))
						{
							RenderParsedRenderGraphView();
						}

						ImGui::EndChild();
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
			}

			ImGui::EndChild();
		}
		ImGui::End();

		if (m_ParsedGraphDirty)
		{
			if (!ParseStructure(true))
			{
				LOG_ERROR("Parsing Error: %s", m_ParsingError.c_str());
				m_ParsingError = "";
			}

			m_ParsedGraphDirty = false;
			m_ParsedGraphRenderDirty = true;
		}
	}

	RenderGraphStructureDesc RenderGraphEditor::CreateRenderGraphStructure(const String& filepath, bool imGuiEnabled)
	{
		RenderGraphStructureDesc returnStructure = {};

		if (LoadFromFile(filepath, imGuiEnabled))
		{
			returnStructure = m_ParsedRenderGraphStructure;
		}

		ResetState();
		
		return returnStructure;
	}

	void RenderGraphEditor::OnButtonReleased(EMouseButton button)
	{
		//imnodes seems to be bugged when releasing a link directly after starting creation, so we check this here
		if (EMouseButton::MOUSE_BUTTON_LEFT)
		{
			if (m_StartedLinkInfo.LinkStarted)
			{
				m_StartedLinkInfo = {};
			}
		}
	}

	void RenderGraphEditor::OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		if (key == EKey::KEY_LEFT_SHIFT && !isRepeat)
		{
			imnodes::PushAttributeFlag(imnodes::AttributeFlags_EnableLinkDetachWithDragClick);
		}
	}

	void RenderGraphEditor::OnKeyReleased(EKey key)
	{
		if (key == EKey::KEY_LEFT_SHIFT)
		{
			imnodes::PopAttributeFlag();
		}
	}

	void RenderGraphEditor::InitDefaultResources()
	{
		m_FilesInShaderDirectory = EnumerateFilesInDirectory("../Assets/Shaders/", true);

		m_FinalOutput.Name						= "FINAL_OUTPUT";
		m_FinalOutput.NodeIndex					= s_NextNodeID++;

		EditorResourceStateGroup externalResourcesGroup = {};
		externalResourcesGroup.Name				= "EXTERNAL_RESOURCES";
		externalResourcesGroup.OutputNodeIndex	= s_NextNodeID++;

		EditorResourceStateGroup temporalResourcesGroup = {};
		temporalResourcesGroup.Name				= "TEMPORAL_RESOURCES";
		temporalResourcesGroup.InputNodeIndex	= s_NextNodeID++;
		temporalResourcesGroup.OutputNodeIndex	= s_NextNodeID++;

		//EditorRenderStage imguiRenderStage = {};
		//imguiRenderStage.Name					= RENDER_GRAPH_IMGUI_STAGE_NAME;
		//imguiRenderStage.NodeIndex				= s_NextNodeID++;
		//imguiRenderStage.InputAttributeIndex	= s_NextAttributeID;
		//imguiRenderStage.Type					= EPipelineStateType::GRAPHICS;
		//imguiRenderStage.CustomRenderer			= true;
		//imguiRenderStage.Enabled				= true;

		//m_RenderStageNameByInputAttributeIndex[imguiRenderStage.InputAttributeIndex] = imguiRenderStage.Name;
		//m_RenderStagesByName[imguiRenderStage.Name] = imguiRenderStage;

		s_NextAttributeID += 2;

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= 1;
			resource.Editable						= false;
			resource.External						= false;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_B8G8R8A8_UNORM;
			m_Resources.push_back(resource);

			m_FinalOutput.BackBufferAttributeIndex					= CreateResourceState(resource.Name, m_FinalOutput.Name, false, ERenderGraphResourceBindingType::NONE).AttributeIndex;
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= FULLSCREEN_QUAD_VERTEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= PER_FRAME_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_LIGHTS_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_MAT_PARAM_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_VERTEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_INDEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_PRIMARY_INSTANCE_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_SECONDARY_INSTANCE_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_INDIRECT_ARGS_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_TLAS;
			resource.Type						= ERenderGraphResourceType::ACCELERATION_STRUCTURE;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_ALBEDO_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_NORMAL_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_AO_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_ROUGHNESS_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_METALLIC_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.push_back(resource);

			externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		m_ResourceStateGroups.resize(NUM_RESOURCE_STATE_GROUPS);
		m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX] = externalResourcesGroup;
		m_ResourceStateGroups[TEMPORAL_RESOURCE_STATE_GROUP_INDEX] = temporalResourcesGroup;
	}

	void RenderGraphEditor::RenderResourceView(float textWidth, float textHeight)
	{
		bool openAddResourcePopup = false;
		bool openEditResourcePopup = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("Add Texture", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = ERenderGraphResourceType::TEXTURE;
				}

				if (ImGui::MenuItem("Add Buffer", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = ERenderGraphResourceType::BUFFER;
				}

				if (ImGui::MenuItem("Add Acceleration Structure", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = ERenderGraphResourceType::ACCELERATION_STRUCTURE;
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Columns(2);

		static int32 selectedResourceIndex			= -1;
		static RenderGraphResourceDesc* pSelectedResource	= nullptr;
		static int32 removedResourceIndex			= -1;
		static RenderGraphResourceDesc* pRemovedResource		= nullptr;

		for (uint32 i = 0; i < m_Resources.size(); i++)
		{
			RenderGraphResourceDesc* pResource = &m_Resources[i];

			if (ImGui::Selectable(pResource->Name.c_str(), selectedResourceIndex == i, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
			{
				selectedResourceIndex	= i;
				pSelectedResource		= pResource;
			}

			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("RESOURCE", &pResource, sizeof(RenderGraphResourceDesc*));
				ImGui::EndDragDropSource();
			}

			if (pResource->Editable)
			{
				ImGui::SameLine();
				if (ImGui::Button(("Edit##" + pResource->Name).c_str()))
				{
					openEditResourcePopup = true;
					m_CurrentlyEditingResource = pResource->Name;
				}

				ImGui::SameLine();
				if (ImGui::Button(("-##" + pResource->Name).c_str()))
				{
					removedResourceIndex	= i;
					pRemovedResource		= pResource;
				}
			}
		}

		if (pSelectedResource != nullptr)
		{
			ImGui::NextColumn();

			String resourceType = RenderGraphResourceTypeToString(pSelectedResource->Type);
			ImGui::Text("Type: %s", resourceType.c_str());

			String subResourceCount;

			if (pSelectedResource->BackBufferBound)
			{
				subResourceCount = "Back Buffer Bound";
			}
			else
			{
				subResourceCount = std::to_string(pSelectedResource->SubResourceCount);
			}

			ImGui::Text("Sub Resource Count: %s", subResourceCount.c_str());

			if (pSelectedResource->Type == ERenderGraphResourceType::TEXTURE)
			{
				if (pSelectedResource->SubResourceCount > 1)
				{
					ImGui::Text("Is of Array Type: %s", pSelectedResource->TextureParams.IsOfArrayType ? "True" : "False");
				}

				int32 textureFormatIndex = TextureFormatToFormatIndex(pSelectedResource->TextureParams.TextureFormat);

				if (textureFormatIndex >= 0)
				{
					ImGui::Text("Texture Format: %s", TEXTURE_FORMAT_NAMES[textureFormatIndex]);
				}
				else
				{
					ImGui::Text("Texture Format: INVALID");
				}
			}
		}

		if (pRemovedResource != nullptr)
		{
			//Update Resource State Groups and Resource States
			for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
			{
				EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);		
				RemoveResourceStateFrom(pRemovedResource->Name, pResourceStateGroup);
			}

			//Update Render Stages and Resource States
			for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
			{
				EditorRenderStageDesc* pRenderStage = &renderStageIt->second;
				RemoveResourceStateFrom(pRemovedResource->Name, pRenderStage);
			}

			m_Resources.erase(m_Resources.begin() + removedResourceIndex);

			removedResourceIndex	= -1;
			pRemovedResource		= nullptr;
		}

		if (openAddResourcePopup)
			ImGui::OpenPopup("Add Resource ##Popup");
		if (openEditResourcePopup)
			ImGui::OpenPopup("Edit Resource ##Popup");
	}

	void RenderGraphEditor::RenderAddResourceView()
	{
		static char resourceNameBuffer[MAX_RESOURCE_NAME_LENGTH];
		static RenderGraphResourceDesc addingResource;
		static bool initialized = false;

		ImGui::SetNextWindowSize(ImVec2(460, 700));
		if (ImGui::BeginPopupModal("Add Resource ##Popup"))
		{
			if (m_CurrentlyAddingResource != ERenderGraphResourceType::NONE)
			{
				if (!initialized)
				{
					addingResource.Type = m_CurrentlyAddingResource;
					initialized = true;
				}

				ImGui::AlignTextToFramePadding();

				InternalRenderEditResourceView(&addingResource, resourceNameBuffer, MAX_RESOURCE_NAME_LENGTH);

				bool done = false;
				bool resourceExists		= FindResource(resourceNameBuffer) != m_Resources.end();
				bool resourceNameEmpty	= resourceNameBuffer[0] == 0;
				bool resourceInvalid	= resourceExists || resourceNameEmpty;

				if (resourceExists)
				{
					ImGui::Text("A resource with that name already exists...");
				}
				else if (resourceNameEmpty)
				{
					ImGui::Text("Resource name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (resourceInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Create"))
				{
					addingResource.Name			= resourceNameBuffer;
					addingResource.Type			= m_CurrentlyAddingResource;
					addingResource.Editable		= true;

					m_Resources.push_back(addingResource);

					if (addingResource.External)
					{
						EditorResourceStateGroup& externalResourcesGroup = m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX];
						externalResourcesGroup.ResourceStateIdents.push_back(CreateResourceState(addingResource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
					}

					done = true;
				}

				if (resourceInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(resourceNameBuffer, MAX_RESOURCE_NAME_LENGTH);
					addingResource				= {};
					m_CurrentlyAddingResource	= ERenderGraphResourceType::NONE;
					initialized					= false;
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderEditResourceView()
	{
		static char resourceNameBuffer[MAX_RESOURCE_NAME_LENGTH];
		static RenderGraphResourceDesc editedResourceCopy;
		static TArray<RenderGraphResourceDesc>::iterator editedResourceIt;
		static bool initialized = false;

		ImGui::SetNextWindowSize(ImVec2(460, 700));
		if (ImGui::BeginPopupModal("Edit Resource ##Popup"))
		{
			if (m_CurrentlyEditingResource != "")
			{
				if (!initialized)
				{
					editedResourceIt = FindResource(m_CurrentlyEditingResource);

					if (editedResourceIt == m_Resources.end())
					{
						ImGui::CloseCurrentPopup();
						ImGui::EndPopup();
						LOG_ERROR("Editing non-existant resource!");
						return;
					}

					editedResourceCopy = *editedResourceIt;

					memcpy(resourceNameBuffer, m_CurrentlyEditingResource.c_str(), m_CurrentlyEditingResource.size());
					initialized = true;
				}

				ImGui::AlignTextToFramePadding();

				InternalRenderEditResourceView(&editedResourceCopy, resourceNameBuffer, MAX_RESOURCE_NAME_LENGTH);

				bool done = false;
				auto existingResourceIt		= FindResource(resourceNameBuffer);
				bool resourceExists			= existingResourceIt != m_Resources.end() && existingResourceIt != editedResourceIt;
				bool resourceNameEmpty		= resourceNameBuffer[0] == 0;
				bool resourceInvalid		= resourceExists || resourceNameEmpty;

				if (resourceExists)
				{
					ImGui::Text("Another resource with that name already exists...");
				}
				else if (resourceNameEmpty)
				{
					ImGui::Text("Resource name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (resourceInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Done"))
				{
					editedResourceCopy.Name				= resourceNameBuffer;

					//Switched from External to not External
					if (editedResourceIt->External && !editedResourceCopy.External)
					{
						EditorResourceStateGroup* pExternalResourcesGroup = &m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX];
						RemoveResourceStateFrom(editedResourceIt->Name, pExternalResourcesGroup);
					}
					else if (!editedResourceIt->External && editedResourceCopy.External)
					{
						EditorResourceStateGroup* pExternalResourcesGroup = &m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX];
						pExternalResourcesGroup->ResourceStateIdents.push_back(CreateResourceState(resourceNameBuffer, pExternalResourcesGroup->Name, false, ERenderGraphResourceBindingType::NONE));
					}

					if (editedResourceCopy.Name != resourceNameBuffer)
					{
						//Update Resource State Groups and Resource States
						for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
						{
							EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

							auto resourceStateIdentIt = pResourceStateGroup->FindResourceStateIdent(editedResourceCopy.Name);

							if (resourceStateIdentIt != pResourceStateGroup->ResourceStateIdents.end())
							{
								int32 attributeIndex = resourceStateIdentIt->AttributeIndex;
								EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];
								pResourceState->ResourceName = resourceNameBuffer;
							}
						}

						//Update Render Stages and Resource States
						for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
						{
							EditorRenderStageDesc* pRenderStage = &renderStageIt->second;

							auto resourceStateIdentIt = pRenderStage->FindResourceStateIdent(editedResourceCopy.Name);

							if (resourceStateIdentIt != pRenderStage->ResourceStateIdents.end())
							{
								int32 attributeIndex = resourceStateIdentIt->AttributeIndex;
								EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];
								pResourceState->ResourceName = resourceNameBuffer;
							}
						}
					}

					(*editedResourceIt) = editedResourceCopy;

					done = true;
				}

				if (resourceInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(resourceNameBuffer, MAX_RESOURCE_NAME_LENGTH);
					m_CurrentlyEditingResource	= "";
					initialized					= false;
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::InternalRenderEditResourceView(RenderGraphResourceDesc* pResource, char* pNameBuffer, int32 nameBufferLength)
	{
		ImGui::Text("Resource Name:      ");
		ImGui::SameLine();
		ImGui::InputText("##Resource Name", pNameBuffer, nameBufferLength, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

		ImGui::Text("External: ");
		ImGui::SameLine();
		ImGui::Checkbox("##External", &pResource->External);

		ImGui::Text("Back Buffer Bound: ");
		ImGui::SameLine();
		ImGui::Checkbox("##Back Buffer Bound", &pResource->BackBufferBound);

		ImGui::Text("Sub Resource Count: ");
		ImGui::SameLine();

		if (pResource->BackBufferBound)
		{
			pResource->SubResourceCount = 1;
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		if (ImGui::InputInt("##Sub Resource Count", &pResource->SubResourceCount, 1, 100))
		{
			pResource->SubResourceCount = glm::clamp<int32>(pResource->SubResourceCount, 1, 1024);
		}

		if (pResource->BackBufferBound)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		switch (pResource->Type)
		{
			case ERenderGraphResourceType::TEXTURE:
			{
				if (pResource->SubResourceCount > 1)
				{
					ImGui::Text("Is of Array Type: ");
					ImGui::SameLine();
					ImGui::Checkbox("##Is of Array Type", &pResource->TextureParams.IsOfArrayType);
				}

				int32 textureFormatIndex = TextureFormatToFormatIndex(pResource->TextureParams.TextureFormat);

				ImGui::Text("Format: ");
				ImGui::SameLine();
				if (ImGui::Combo("##Resource Format", &textureFormatIndex, TEXTURE_FORMAT_NAMES, ARR_SIZE(TEXTURE_FORMAT_NAMES)))
				{
					pResource->TextureParams.TextureFormat = TextureFormatIndexToFormat(textureFormatIndex);
				}

				if (!pResource->External)
				{
					ImGuiStyle& style = ImGui::GetStyle();
					static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[0]).x + ImGui::GetFrameHeight() + 10;

					int32 textureDimensionTypeX = DimensionTypeToDimensionTypeIndex(pResource->TextureParams.XDimType);
					int32 textureDimensionTypeY = DimensionTypeToDimensionTypeIndex(pResource->TextureParams.YDimType);

					ImGui::Text("Width: ");
					ImGui::SameLine();
					ImGui::PushItemWidth(maxOptionTextSize);
					if (ImGui::Combo("##Texture X Option", &textureDimensionTypeX, DIMENSION_NAMES, 3))
					{
						pResource->TextureParams.XDimType = DimensionTypeIndexToDimensionType(textureDimensionTypeX);
					}
					ImGui::PopItemWidth();

					if (pResource->TextureParams.XDimType == ERenderGraphDimensionType::CONSTANT || pResource->TextureParams.XDimType == ERenderGraphDimensionType::RELATIVE)
					{
						ImGui::SameLine();
						ImGui::InputFloat("##Texture X Variable", &pResource->TextureParams.XDimVariable);
					}

					ImGui::Text("Height: ");
					ImGui::SameLine();
					ImGui::PushItemWidth(maxOptionTextSize);
					if (ImGui::Combo("##Texture Y Option", &textureDimensionTypeY, DIMENSION_NAMES, 3))
					{
						pResource->TextureParams.YDimType = DimensionTypeIndexToDimensionType(textureDimensionTypeY);
					}
					ImGui::PopItemWidth();

					if (pResource->TextureParams.YDimType == ERenderGraphDimensionType::CONSTANT || pResource->TextureParams.YDimType == ERenderGraphDimensionType::RELATIVE)
					{
						ImGui::SameLine();
						ImGui::InputFloat("##Texture Y Variable", &pResource->TextureParams.YDimVariable);
					}

					ImGui::Text("Sample Count: ");
					ImGui::SameLine();
					ImGui::InputInt("##Texture Sample Count", &pResource->TextureParams.SampleCount);

					ImGui::Text("Miplevel Count: ");
					ImGui::SameLine();
					ImGui::InputInt("##Miplevel Count", &pResource->TextureParams.MiplevelCount);

					int32 samplerTypeIndex = SamplerTypeToSamplerTypeIndex(pResource->TextureParams.SamplerType);

					ImGui::Text("Sampler Type: ");
					ImGui::SameLine();
					if (ImGui::Combo("##Sampler Type", &samplerTypeIndex, SAMPLER_NAMES, ARR_SIZE(SAMPLER_NAMES)))
					{
						pResource->TextureParams.SamplerType = SamplerTypeIndexToSamplerType(samplerTypeIndex);
					}

					int32 memoryTypeIndex = MemoryTypeToMemoryTypeIndex(pResource->MemoryType);

					ImGui::Text("Memory Type: ");
					ImGui::SameLine();
					if (ImGui::Combo("##Memory Type", &memoryTypeIndex, MEMORY_TYPE_NAMES, ARR_SIZE(MEMORY_TYPE_NAMES)))
					{
						pResource->MemoryType = MemoryTypeIndexToMemoryType(memoryTypeIndex);
					}
				}
				break;
			}
			case ERenderGraphResourceType::BUFFER:
			{
				if (!pResource->External)
				{
					ImGuiStyle& style = ImGui::GetStyle();
					static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[0]).x + ImGui::GetFrameHeight() + 10;

					int32 bufferSizeType = DimensionTypeToDimensionTypeIndex(pResource->BufferParams.SizeType);

					ImGui::Text("Size: ");
					ImGui::SameLine();
					ImGui::PushItemWidth(maxOptionTextSize);
					if (ImGui::Combo("##Buffer Size Option", &bufferSizeType, DIMENSION_NAMES, 2))
					{
						pResource->BufferParams.SizeType = DimensionTypeIndexToDimensionType(bufferSizeType);
					}
					ImGui::PopItemWidth();

					if (pResource->BufferParams.SizeType == ERenderGraphDimensionType::CONSTANT)
					{
						ImGui::SameLine();
						ImGui::InputInt("##Buffer Size", &pResource->BufferParams.Size);
					}

					int32 memoryTypeIndex = MemoryTypeToMemoryTypeIndex(pResource->MemoryType);

					ImGui::Text("Memory Type: ");
					ImGui::SameLine();
					if (ImGui::Combo("##Memory Type", &memoryTypeIndex, MEMORY_TYPE_NAMES, ARR_SIZE(MEMORY_TYPE_NAMES)))
					{
						pResource->MemoryType = MemoryTypeIndexToMemoryType(memoryTypeIndex);
					}
				}
				break;
			}
			case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
			{
				break;
			}
		}
	}

	void RenderGraphEditor::RenderShaderView(float textWidth, float textHeight)
	{
		UNREFERENCED_VARIABLE(textHeight);

		static int32 selectedResourceIndex = -1;
		
		for (auto fileIt = m_FilesInShaderDirectory.begin(); fileIt != m_FilesInShaderDirectory.end(); fileIt++)
		{
			int32 index = std::distance(m_FilesInShaderDirectory.begin(), fileIt);
			const String* pFilename = &(*fileIt);

			//if (pFilename->find(".glsl") != String::npos)
			{
				if (ImGui::Selectable(pFilename->c_str(), selectedResourceIndex == index, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
				{
					selectedResourceIndex = index;
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("SHADER", &pFilename, sizeof(const String*));
					ImGui::EndDragDropSource();
				}
			}
		}
	}

	void RenderGraphEditor::RenderGraphView()
	{
		bool openAddRenderStagePopup = false;
		bool openSaveRenderStagePopup = false;
		bool openLoadRenderStagePopup = false;

		String renderStageToDelete = "";

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("Save", NULL, nullptr))
				{
					openSaveRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Load", NULL, nullptr))
				{
					openLoadRenderStagePopup = true;
				}

				/*ImGui::NewLine();

				if (ImGui::MenuItem("Apply", NULL, nullptr))
				{
					RefactoredRenderGraph* pTest = DBG_NEW RefactoredRenderGraph(RenderSystem::GetDevice());

					RenderGraphDesc renderGraphDesc = {};
					renderGraphDesc.MaxTexturesPerDescriptorSet = 256;
					renderGraphDesc.pParsedRenderGraphStructure = &m_ParsedRenderGraphStructure;

					pTest->Init(&renderGraphDesc);
				}*/

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Add Graphics Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::GRAPHICS;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Compute Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::COMPUTE;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Ray Tracing Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::RAY_TRACING;
					openAddRenderStagePopup = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		//Resource State Groups
		for (uint32 resourceStateGroupIndex = 0; resourceStateGroupIndex < m_ResourceStateGroups.size(); resourceStateGroupIndex++)
		{
			EditorResourceStateGroup* pResourceStateGroup = &m_ResourceStateGroups[resourceStateGroupIndex];

			imnodes::BeginNode(pResourceStateGroup->OutputNodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::TextUnformatted((pResourceStateGroup->Name + "_OUTPUT").c_str());
			imnodes::EndNodeTitleBar();

			String resourceStateToRemove = "";

			for (const EditorResourceStateIdent& resourceStateIdent : pResourceStateGroup->ResourceStateIdents)
			{
				uint32 primaryAttributeIndex	= resourceStateIdent.AttributeIndex / 2;
				uint32 inputAttributeIndex		= resourceStateIdent.AttributeIndex;
				uint32 outputAttributeIndex		= inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, nullptr, pResourceState, inputAttributeIndex);
				imnodes::BeginOutputAttribute(outputAttributeIndex);
				ImGui::Text(pResourceState->ResourceName.c_str());
				ImGui::SameLine();
				if (pResourceState->Removable)
				{
					if (ImGui::Button("-"))
					{
						resourceStateToRemove = pResourceState->ResourceName;
					}
				}
				imnodes::EndOutputAttribute();
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, nullptr, pResourceState, inputAttributeIndex);
			}

			if (resourceStateGroupIndex != EXTERNAL_RESOURCE_STATE_GROUP_INDEX)
			{
				ImGui::Button("Drag Resource Here");

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

					if (pPayload != nullptr)
					{
						RenderGraphResourceDesc* pResource = *reinterpret_cast<RenderGraphResourceDesc**>(pPayload->Data);

						if (pResourceStateGroup->FindResourceStateIdent(pResource->Name) == pResourceStateGroup->ResourceStateIdents.end())
						{
							pResourceStateGroup->ResourceStateIdents.push_back(CreateResourceState(pResource->Name, pResourceStateGroup->Name, true, ERenderGraphResourceBindingType::NONE));
							m_ParsedGraphDirty = true;
						}
					}

					ImGui::EndDragDropTarget();
				}
			}

			imnodes::EndNode();

			//Remove resource if "-" button pressed
			if (resourceStateToRemove.size() > 0)
			{
				RemoveResourceStateFrom(resourceStateToRemove, pResourceStateGroup);
			}

			//Temporal Resource State Group has Output and Input Stages
			if (resourceStateGroupIndex == TEMPORAL_RESOURCE_STATE_GROUP_INDEX)
			{
				imnodes::BeginNode(pResourceStateGroup->InputNodeIndex);

				imnodes::BeginNodeTitleBar();
				ImGui::TextUnformatted((pResourceStateGroup->Name + "_INPUT").c_str());
				imnodes::EndNodeTitleBar();

				String resourceStateToRemove = "";

				for (const EditorResourceStateIdent& resourceStateIdent : pResourceStateGroup->ResourceStateIdents)
				{
					uint32 primaryAttributeIndex	= resourceStateIdent.AttributeIndex / 2;
					uint32 inputAttributeIndex		= resourceStateIdent.AttributeIndex;
					uint32 outputAttributeIndex		= inputAttributeIndex + 1;
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

					PushPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResourceState, inputAttributeIndex);
					imnodes::BeginInputAttribute(inputAttributeIndex);
					ImGui::Text(pResourceState->ResourceName.c_str());
					ImGui::SameLine();
					if (pResourceState->Removable)
					{
						if (ImGui::Button("-"))
						{
							resourceStateToRemove = pResourceState->ResourceName;
						}
					}
					imnodes::EndInputAttribute();
					PopPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResourceState, inputAttributeIndex);
				}

				ImGui::Button("Drag Resource Here");

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

					if (pPayload != nullptr)
					{
						RenderGraphResourceDesc* pResource = *reinterpret_cast<RenderGraphResourceDesc**>(pPayload->Data);

						if (pResourceStateGroup->FindResourceStateIdent(pResource->Name) == pResourceStateGroup->ResourceStateIdents.end())
						{
							pResourceStateGroup->ResourceStateIdents.push_back(CreateResourceState(pResource->Name, pResourceStateGroup->Name, true, ERenderGraphResourceBindingType::NONE));
							m_ParsedGraphDirty = true;
						}
					}

					ImGui::EndDragDropTarget();
				}

				imnodes::EndNode();

				//Remove resource if "-" button pressed
				if (resourceStateToRemove.size() > 0)
				{
					RemoveResourceStateFrom(resourceStateToRemove, pResourceStateGroup);
				}
			}
		}

		//Final Output
		{
			imnodes::BeginNode(m_FinalOutput.NodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::TextUnformatted("FINAL_OUTPUT");
			imnodes::EndNodeTitleBar();

			uint32 primaryAttributeIndex	= m_FinalOutput.BackBufferAttributeIndex / 2;
			uint32 inputAttributeIndex		= m_FinalOutput.BackBufferAttributeIndex;
			uint32 outputAttributeIndex		= inputAttributeIndex + 1;
			EditorRenderGraphResourceState* pResource = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

			PushPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResource, inputAttributeIndex);
			imnodes::BeginInputAttribute(inputAttributeIndex);
			ImGui::Text(pResource->ResourceName.c_str());
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::INPUT, nullptr, pResource, inputAttributeIndex);

			imnodes::EndNode();
		}

		//Render Stages
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStageDesc* pRenderStage = &renderStageIt->second;
			bool hasDepthAttachment = false;

			int32 moveResourceStateAttributeIndex	= -1;
			int32 moveResourceStateMoveAddition		= 0;

			imnodes::BeginNode(pRenderStage->NodeIndex);

			String renderStageType = RenderStageTypeToString(pRenderStage->Type);
			
			imnodes::BeginNodeTitleBar();
			ImGui::Text("%s : [%s]", pRenderStage->Name.c_str(), renderStageType.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				renderStageToDelete = pRenderStage->Name;
			}
			ImGui::TextUnformatted("Enabled: ");
			ImGui::SameLine();
			if (ImGui::Checkbox("##Render Stage Enabled Checkbox", &pRenderStage->Enabled)) m_ParsedGraphDirty = true;
			ImGui::Text("Weight: %d", pRenderStage->Weight);

			ImGui::Text("Allow Overriding of Binding Types:");
			ImGui::SameLine();
			ImGui::Checkbox(("##Override Recommended Binding Types" + pRenderStage->Name).c_str(), &pRenderStage->OverrideRecommendedBindingType);

			imnodes::EndNodeTitleBar();

			String resourceStateToRemove = "";

			//Render Resource State
			for (uint32 resourceStateLocalIndex = 0; resourceStateLocalIndex < pRenderStage->ResourceStateIdents.size(); resourceStateLocalIndex++)
			{
				const EditorResourceStateIdent* pResourceStateIdent = &pRenderStage->ResourceStateIdents[resourceStateLocalIndex];
				int32 primaryAttributeIndex		= pResourceStateIdent->AttributeIndex / 2;
				int32 inputAttributeIndex		= pResourceStateIdent->AttributeIndex;
				int32 outputAttributeIndex		= inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				auto resourceIt = FindResource(pResourceState->ResourceName);

				if (resourceIt == m_Resources.end())
				{
					LOG_ERROR("[RenderGraphEditor]: Resource with name \"%s\" could not be found when calculating resource state binding types", pResourceState->ResourceName.c_str());
					return;
				}

				RenderGraphResourceDesc* pResource = &(*resourceIt);

				PushPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, inputAttributeIndex);
				imnodes::BeginInputAttribute(inputAttributeIndex);
				ImGui::Text(pResourceState->ResourceName.c_str());
				imnodes::EndInputAttribute();
				PopPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, inputAttributeIndex);

				ImGui::SameLine();

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, pRenderStage, pResourceState, outputAttributeIndex);
				imnodes::BeginOutputAttribute(outputAttributeIndex);
				if (pResourceState->Removable)
				{
					if (ImGui::Button("-"))
					{
						resourceStateToRemove = pResourceState->ResourceName;
					}
				}
				else
				{
					ImGui::InvisibleButton("##Resouce State Invisible Button", ImGui::CalcTextSize("-"));
				}

				if (resourceStateLocalIndex > 0)
				{
					ImGui::SameLine();

					if (ImGui::ArrowButton("##Move Resource State Up Button", ImGuiDir_Up))
					{
						moveResourceStateAttributeIndex = primaryAttributeIndex;
						moveResourceStateMoveAddition = -1;
					}
				}

				if (resourceStateLocalIndex < pRenderStage->ResourceStateIdents.size() - 1)
				{
					ImGui::SameLine();

					if (ImGui::ArrowButton("##Move Resource State Down Button", ImGuiDir_Down))
					{
						moveResourceStateAttributeIndex = primaryAttributeIndex;
						moveResourceStateMoveAddition = 1;
					}
				}

				imnodes::EndOutputAttribute();
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, pRenderStage, pResourceState, outputAttributeIndex);

				TArray<ERenderGraphResourceBindingType> bindingTypes;
				TArray<const char*> bindingTypeNames;
				CalculateResourceStateBindingTypes(pRenderStage, pResource, pResourceState, bindingTypes, bindingTypeNames);

				if (bindingTypes.size() > 0)
				{
					int32 selectedItem = 0;

					if (pResourceState->BindingType != ERenderGraphResourceBindingType::NONE)
					{
						auto bindingTypeIt = std::find(bindingTypes.begin(), bindingTypes.end(), pResourceState->BindingType);

						if (bindingTypeIt != bindingTypes.end())
						{
							selectedItem = std::distance(bindingTypes.begin(), bindingTypeIt);
						}
						else
						{
							pResourceState->BindingType = bindingTypes[0];
						}
					}

					ImGui::Text("\tBinding:");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(ImGui::CalcTextSize("COMBINED SAMPLER").x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
					if (ImGui::BeginCombo(("##Binding List" + pResourceState->ResourceName).c_str(), bindingTypeNames[selectedItem]))
					{
						for (uint32 bt = 0; bt < bindingTypeNames.size(); bt++)
						{
							const bool is_selected = (selectedItem == bt);
							if (ImGui::Selectable(bindingTypeNames[bt], is_selected))
							{
								selectedItem = bt;
								pResourceState->BindingType = bindingTypes[selectedItem];
								m_ParsedGraphDirty = true;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				if (pResourceState->BindingType == ERenderGraphResourceBindingType::ATTACHMENT && pResource->Type == ERenderGraphResourceType::TEXTURE && pResource->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
					hasDepthAttachment = true;
			}

			PushPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, pRenderStage, nullptr , -1);
			imnodes::BeginInputAttribute(pRenderStage->InputAttributeIndex);
			ImGui::Text("New Input");
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, pRenderStage, nullptr, -1);

			ImGui::Button("Drag Resource Here");

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

				if (pPayload != nullptr)
				{
					RenderGraphResourceDesc* pResource = *reinterpret_cast<RenderGraphResourceDesc**>(pPayload->Data);

					if (pRenderStage->FindResourceStateIdent(pResource->Name) == pRenderStage->ResourceStateIdents.end())
					{
						pRenderStage->ResourceStateIdents.push_back(CreateResourceState(pResource->Name, pRenderStage->Name, true, ERenderGraphResourceBindingType::NONE));
						m_ParsedGraphDirty = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			//If Graphics, Render Draw Type and Render Pass options 
			if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
			{
				if (hasDepthAttachment)
				{
					ImGui::Text("Depth Testing Enabled:");
					ImGui::SameLine();
					ImGui::Checkbox("##Depth Testing Enabled", &pRenderStage->Graphics.DepthTestEnabled);
				}
				else
				{
					pRenderStage->Graphics.DepthTestEnabled = false;
				}

				TArray<ERenderStageDrawType> drawTypes							= { ERenderStageDrawType::SCENE_INDIRECT, ERenderStageDrawType::FULLSCREEN_QUAD };
				TArray<const char*> drawTypeNames								= { "SCENE INDIRECT", "FULLSCREEN QUAD" };
				auto selectedDrawTypeIt											= std::find(drawTypes.begin(), drawTypes.end(), pRenderStage->Graphics.DrawType);
				int32 selectedDrawType											= 0;
				if (selectedDrawTypeIt != drawTypes.end()) selectedDrawType		= std::distance(drawTypes.begin(), selectedDrawTypeIt);

				ImGui::Text(("\tDraw Type:"));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::CalcTextSize("FULLSCREEN QUAD").x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
				if (ImGui::BeginCombo(("##Draw Type" + pRenderStage->Name).c_str(), drawTypeNames[selectedDrawType]))
				{
					for (uint32 dt = 0; dt < drawTypeNames.size(); dt++)
					{
						const bool is_selected = (selectedDrawType == dt);
						if (ImGui::Selectable(drawTypeNames[dt], is_selected))
						{
							selectedDrawType = dt;
							pRenderStage->Graphics.DrawType = drawTypes[selectedDrawType];
							m_ParsedGraphDirty = true;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				if (pRenderStage->Graphics.DrawType == ERenderStageDrawType::SCENE_INDIRECT)
				{
					//Index Buffer
					{
						EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[pRenderStage->Graphics.IndexBufferAttributeIndex / 2];

						PushPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, -1);
						imnodes::BeginInputAttribute(pRenderStage->Graphics.IndexBufferAttributeIndex);
						ImGui::Text("Index Buffer");

						if (pResourceState->ResourceName.size() > 0)
						{
							ImGui::Text(pResourceState->ResourceName.c_str());
							ImGui::SameLine();

							if (pResourceState->Removable)
							{
								if (ImGui::Button("-"))
								{
									pResourceState->ResourceName = "";
									DestroyLink(pResourceState->InputLinkIndex);
								}
							}
						}
						imnodes::EndInputAttribute();
						PopPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, -1);
					}
					
					//Indirect Args Buffer
					{
						EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[pRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];

						PushPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, -1);
						imnodes::BeginInputAttribute(pRenderStage->Graphics.IndirectArgsBufferAttributeIndex);
						ImGui::Text("Indirect Args Buffer");

						if (pResourceState->ResourceName.size() > 0)
						{
							ImGui::Text(pResourceState->ResourceName.c_str());
							ImGui::SameLine();

							if (pResourceState->Removable)
							{
								if (ImGui::Button("-"))
								{
									pResourceState->ResourceName = "";
									DestroyLink(pResourceState->InputLinkIndex);
								}
							}
						}
						imnodes::EndInputAttribute();
						PopPinColorIfNeeded(EEditorPinType::INPUT, pRenderStage, pResourceState, -1);
					}
				}
			}

			//Render Shader Boxes
			if (!pRenderStage->CustomRenderer)
			{
				RenderShaderBoxes(pRenderStage);
			}

			imnodes::EndNode();

			//Remove resource if "-" button pressed
			if (resourceStateToRemove.size() > 0)
			{
				RemoveResourceStateFrom(resourceStateToRemove, pRenderStage);
			}

			//Move Resource State
			if (moveResourceStateAttributeIndex != -1)
			{
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[moveResourceStateAttributeIndex];

				auto resourceStateMoveIt	= pRenderStage->FindResourceStateIdent(pResourceState->ResourceName);
				auto resourceStateSwapIt	= resourceStateMoveIt + moveResourceStateMoveAddition;
				std::iter_swap(resourceStateMoveIt, resourceStateSwapIt);
			}
		}

		//Render Links
		for (auto linkIt = m_ResourceStateLinksByLinkIndex.begin(); linkIt != m_ResourceStateLinksByLinkIndex.end(); linkIt++)
		{
			EditorRenderGraphResourceLink* pLink = &linkIt->second;

			imnodes::Link(pLink->LinkIndex, pLink->SrcAttributeIndex, pLink->DstAttributeIndex);
		}

		imnodes::EndNodeEditor();

		//Check for newly destroyed Render Stage
		if (!renderStageToDelete.empty())
		{
			auto renderStageByNameIt = m_RenderStagesByName.find(renderStageToDelete);

			EditorRenderStageDesc* pRenderStage = &renderStageByNameIt->second;

			if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
			{
				if (pRenderStage->Graphics.IndexBufferAttributeIndex != -1)
				{
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[pRenderStage->Graphics.IndexBufferAttributeIndex / 2];
					DestroyLink(pResourceState->InputLinkIndex);

					m_ResourceStatesByHalfAttributeIndex.erase(pRenderStage->Graphics.IndexBufferAttributeIndex);
					m_ParsedGraphDirty = true;
				}

				if (pRenderStage->Graphics.IndexBufferAttributeIndex != -1)
				{
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[pRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];
					DestroyLink(pResourceState->InputLinkIndex);

					m_ResourceStatesByHalfAttributeIndex.erase(pRenderStage->Graphics.IndexBufferAttributeIndex);
					m_ParsedGraphDirty = true;
				}
			}

			for (EditorResourceStateIdent& resourceStateIdent : pRenderStage->ResourceStateIdents)
			{
				int32 resourceAttributeIndex	= resourceStateIdent.AttributeIndex;
				int32 primaryAttributeIndex		= resourceAttributeIndex / 2;
				int32 inputAttributeIndex		= resourceAttributeIndex;
				int32 outputAttributeIndex		= resourceAttributeIndex + 1;

				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				DestroyLink(pResourceState->InputLinkIndex);

				//Copy so that DestroyLink wont delete from set we're iterating through
				TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
				for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
				{
					int32 linkToBeDestroyedIndex = *outputLinkIt;
					DestroyLink(linkToBeDestroyedIndex);
				}

				m_ResourceStatesByHalfAttributeIndex.erase(primaryAttributeIndex);
				m_ParsedGraphDirty = true;
			}

			pRenderStage->ResourceStateIdents.clear();

			m_RenderStageNameByInputAttributeIndex.erase(pRenderStage->InputAttributeIndex);
			m_RenderStagesByName.erase(renderStageByNameIt);
		}

		int32 linkStartAttributeID		= -1;
		
		if (imnodes::IsLinkStarted(&linkStartAttributeID))
		{
			m_StartedLinkInfo.LinkStarted				= true;
			m_StartedLinkInfo.LinkStartAttributeID		= linkStartAttributeID;
			m_StartedLinkInfo.LinkStartedOnInputPin		= linkStartAttributeID % 2 == 0;
			m_StartedLinkInfo.LinkStartedOnResource		= m_ResourceStatesByHalfAttributeIndex[linkStartAttributeID / 2].ResourceName;
		}

		if (imnodes::IsLinkDropped())
		{
			m_StartedLinkInfo = {};
		}

		//Check for newly created Links
		int32 srcAttributeIndex = 0;
		int32 dstAttributeIndex = 0;
		if (imnodes::IsLinkCreated(&srcAttributeIndex, &dstAttributeIndex))
		{
			if (CheckLinkValid(&srcAttributeIndex, &dstAttributeIndex))
			{
				//Check if Render Stage Input Attribute
				auto renderStageNameIt = m_RenderStageNameByInputAttributeIndex.find(dstAttributeIndex);
				if (renderStageNameIt != m_RenderStageNameByInputAttributeIndex.end())
				{
					EditorRenderStageDesc* pRenderStage = &m_RenderStagesByName[renderStageNameIt->second];
					auto resourceIt = FindResource(m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2].ResourceName);

					if (resourceIt != m_Resources.end())
					{
						auto resourceStateIdentIt = pRenderStage->FindResourceStateIdent(resourceIt->Name);

						if (resourceStateIdentIt == pRenderStage->ResourceStateIdents.end())
						{
							pRenderStage->ResourceStateIdents.push_back(CreateResourceState(resourceIt->Name, pRenderStage->Name, true, ERenderGraphResourceBindingType::NONE));
							resourceStateIdentIt = pRenderStage->ResourceStateIdents.begin() + (pRenderStage->ResourceStateIdents.size() - 1);
						}

						dstAttributeIndex = resourceStateIdentIt->AttributeIndex;
					}
				}

				EditorRenderGraphResourceState* pSrcResourceState = &m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2];
				EditorRenderGraphResourceState* pDstResourceState = &m_ResourceStatesByHalfAttributeIndex[dstAttributeIndex / 2];

				auto srcResourceIt = FindResource(pSrcResourceState->ResourceName);

				if (srcResourceIt != m_Resources.end())
				{
					bool allowedDrawResource = pDstResourceState->BindingType == ERenderGraphResourceBindingType::DRAW_RESOURCE && srcResourceIt->Type == ERenderGraphResourceType::BUFFER;

					if (pSrcResourceState->ResourceName == pDstResourceState->ResourceName || allowedDrawResource)
					{
						//Destroy old link
						if (pDstResourceState->InputLinkIndex != -1)
						{
							int32 linkToBeDestroyedIndex = pDstResourceState->InputLinkIndex;
							DestroyLink(linkToBeDestroyedIndex);
						}

						//If this is a Draw Resource, rename it
						if (allowedDrawResource)
						{
							pDstResourceState->ResourceName = pSrcResourceState->ResourceName;
						}

						EditorRenderGraphResourceLink newLink = {};
						newLink.LinkIndex = s_NextLinkID++;
						newLink.SrcAttributeIndex = srcAttributeIndex;
						newLink.DstAttributeIndex = dstAttributeIndex;
						m_ResourceStateLinksByLinkIndex[newLink.LinkIndex] = newLink;

						pDstResourceState->InputLinkIndex = newLink.LinkIndex;
						pSrcResourceState->OutputLinkIndices.insert(newLink.LinkIndex);
						m_ParsedGraphDirty = true;
					}
				}
			}

			m_StartedLinkInfo = {};
		}

		//Check for newly destroyed links
		int32 linkIndex = 0;
		if (imnodes::IsLinkDestroyed(&linkIndex))
		{
			DestroyLink(linkIndex);

			m_StartedLinkInfo = {};
		}

		if		(openAddRenderStagePopup)	ImGui::OpenPopup("Add Render Stage ##Popup");
		else if (openSaveRenderStagePopup)	ImGui::OpenPopup("Save Render Graph ##Popup");
		else if (openLoadRenderStagePopup)	ImGui::OpenPopup("Load Render Graph ##Popup");
	}

	void RenderGraphEditor::RenderAddRenderStageView()
	{
		constexpr const int32 RENDER_STAGE_NAME_BUFFER_LENGTH = 256;
		static char renderStageNameBuffer[RENDER_STAGE_NAME_BUFFER_LENGTH];
		static bool customRenderer = false;

		static int32	selectedXOption	= 2;
		static int32	selectedYOption	= 2;
		static int32	selectedZOption	= 0;

		static float32	xVariable = 1.0f;
		static float32	yVariable = 1.0f;
		static float32	zVariable = 1.0f;

		ImGui::SetNextWindowSize(ImVec2(360, 500), ImGuiCond_Once);
		if (ImGui::BeginPopupModal("Add Render Stage ##Popup"))
		{
			if (m_CurrentlyAddingRenderStage != EPipelineStateType::NONE)
			{
				ImGui::AlignTextToFramePadding();

				ImGui::Text("Render Stage Name:");
				ImGui::SameLine();
				ImGui::InputText("##Render Stage Name", renderStageNameBuffer, RENDER_STAGE_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				ImGui::Text("Custom Renderer:  ");
				ImGui::SameLine();
				ImGui::Checkbox("##Render Stage Custom Renderer", &customRenderer);

				//Render Pipeline State specific options
				if (!customRenderer)
				{
					if (m_CurrentlyAddingRenderStage == EPipelineStateType::GRAPHICS)
					{
						ImGui::Text("Dimensions");

						ImGuiStyle& style = ImGui::GetStyle();
						static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[0]).x + ImGui::GetFrameHeight() + 10;

						ImGui::Text("Width:  ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage X Option", &selectedXOption, DIMENSION_NAMES, 3);
						ImGui::PopItemWidth();

						if (selectedXOption == 0 || selectedXOption == 2)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage X Variable", &xVariable);
						}

						ImGui::Text("Height: ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage Y Option", &selectedYOption, DIMENSION_NAMES, 3);
						ImGui::PopItemWidth();

						if (selectedYOption == 0 || selectedYOption == 2)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage Y Variable", &yVariable);
						}
					}
					else if (m_CurrentlyAddingRenderStage == EPipelineStateType::COMPUTE)
					{
						ImGui::Text("Work Group Size");

						ImGuiStyle& style = ImGui::GetStyle();
						static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[3]).x + ImGui::GetFrameHeight() + 10;

						ImGui::Text("X: ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage X Option", &selectedXOption, DIMENSION_NAMES, 4);
						ImGui::PopItemWidth();

						if (selectedXOption == 0 || selectedXOption == 2 || selectedXOption == 3)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage X Variable", &xVariable);
						}

						if (selectedXOption != 3)
						{
							ImGui::Text("Y: ");
							ImGui::SameLine();
							ImGui::PushItemWidth(maxOptionTextSize);
							ImGui::Combo("##Render Stage Y Option", &selectedYOption, DIMENSION_NAMES, 3);
							ImGui::PopItemWidth();

							if (selectedYOption == 0 || selectedYOption == 2)
							{
								ImGui::SameLine();
								ImGui::InputFloat("##Render Stage Y Variable", &yVariable);
							}

							ImGui::Text("Z: ");
							ImGui::SameLine();
							ImGui::PushItemWidth(maxOptionTextSize);
							ImGui::Combo("##Render Stage Z Option", &selectedZOption, DIMENSION_NAMES, 2);
							ImGui::PopItemWidth();

							if (selectedZOption == 0 || selectedZOption == 2)
							{
								ImGui::SameLine();
								ImGui::InputFloat("##Render Stage Z Variable", &zVariable);
							}
						}
					}
					else if (m_CurrentlyAddingRenderStage == EPipelineStateType::RAY_TRACING)
					{
						ImGui::Text("Ray Gen. Dimensions");

						ImGuiStyle& style = ImGui::GetStyle();
						static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[0]).x + ImGui::GetFrameHeight() + 10;

						ImGui::Text("Wdith: ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage X Option", &selectedXOption, DIMENSION_NAMES, 3);
						ImGui::PopItemWidth();

						if (selectedXOption == 0 || selectedXOption == 2)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage X Variable", &xVariable);
						}

						ImGui::Text("Height: ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage Y Option", &selectedYOption, DIMENSION_NAMES, 3);
						ImGui::PopItemWidth();

						if (selectedYOption == 0 || selectedYOption == 2)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage Y Variable", &yVariable);
						}

						ImGui::Text("Depth: ");
						ImGui::SameLine();
						ImGui::PushItemWidth(maxOptionTextSize);
						ImGui::Combo("##Render Stage Z Option", &selectedZOption, DIMENSION_NAMES, 2);
						ImGui::PopItemWidth();

						if (selectedZOption == 0 || selectedZOption == 2)
						{
							ImGui::SameLine();
							ImGui::InputFloat("##Render Stage Z Variable", &zVariable);
						}
					}
				}

				bool done = false;
				bool renderStageExists = m_RenderStagesByName.find(renderStageNameBuffer) != m_RenderStagesByName.end();
				bool renderStageNameEmpty = renderStageNameBuffer[0] == 0;
				bool renderStageInvalid = renderStageExists || renderStageNameEmpty;

				if (renderStageExists)
				{
					ImGui::Text("A render stage with that name already exists...");
				}
				else if (renderStageNameEmpty)
				{
					ImGui::Text("Render Stage name empty...");
				}

				if (ImGui::Button("Close"))
				{
					done = true;
				}

				ImGui::SameLine();

				if (renderStageInvalid)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("Create"))
				{
					EditorRenderStageDesc newRenderStage = {};
					newRenderStage.Name					= renderStageNameBuffer;
					newRenderStage.NodeIndex			= s_NextNodeID++;
					newRenderStage.InputAttributeIndex	= s_NextAttributeID;
					newRenderStage.Type					= m_CurrentlyAddingRenderStage;
					newRenderStage.CustomRenderer		= customRenderer;
					newRenderStage.Enabled				= true;

					newRenderStage.Parameters.XDimType		= DimensionTypeIndexToDimensionType(selectedXOption);
					newRenderStage.Parameters.YDimType		= DimensionTypeIndexToDimensionType(selectedYOption);
					newRenderStage.Parameters.ZDimType		= DimensionTypeIndexToDimensionType(selectedZOption);

					newRenderStage.Parameters.XDimVariable	= xVariable;
					newRenderStage.Parameters.YDimVariable	= yVariable;
					newRenderStage.Parameters.ZDimVariable	= zVariable;

					s_NextAttributeID += 2;

					if (m_CurrentlyAddingRenderStage == EPipelineStateType::GRAPHICS)
					{
						newRenderStage.Graphics.DrawType							= ERenderStageDrawType::FULLSCREEN_QUAD;
						newRenderStage.Graphics.IndexBufferAttributeIndex			= CreateResourceState("",	newRenderStage.Name, true, ERenderGraphResourceBindingType::DRAW_RESOURCE).AttributeIndex;
						newRenderStage.Graphics.IndirectArgsBufferAttributeIndex	= CreateResourceState("",	newRenderStage.Name, true, ERenderGraphResourceBindingType::DRAW_RESOURCE).AttributeIndex;
					}

					m_RenderStageNameByInputAttributeIndex[newRenderStage.InputAttributeIndex] = newRenderStage.Name;
					m_RenderStagesByName[newRenderStage.Name] = newRenderStage;

					done = true;
				}

				if (renderStageInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(renderStageNameBuffer, RENDER_STAGE_NAME_BUFFER_LENGTH);
					customRenderer = false;
					selectedXOption = 2;
					selectedYOption = 2;
					selectedZOption = 0;
					xVariable = 1.0f;
					yVariable = 1.0f;
					zVariable = 1.0f;
					m_CurrentlyAddingRenderStage = EPipelineStateType::NONE;
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderSaveRenderGraphView()
	{
		constexpr const int32 RENDER_GRAPH_NAME_BUFFER_LENGTH = 256;
		static char renderGraphNameBuffer[RENDER_GRAPH_NAME_BUFFER_LENGTH];

		ImGui::SetNextWindowSize(ImVec2(360, 120));
		if (ImGui::BeginPopupModal("Save Render Graph ##Popup"))
		{
			ImGui::Text("Render Graph Name:");
			ImGui::SameLine();
			ImGui::InputText("##Render Graph Name", renderGraphNameBuffer, RENDER_GRAPH_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

			bool done = false;
			bool renderGraphNameEmpty = renderGraphNameBuffer[0] == 0;

			if (renderGraphNameEmpty)
			{
				ImGui::Text("Render Graph name empty...");
			}

			if (ImGui::Button("Close"))
			{
				done = true;
			}

			ImGui::SameLine();

			if (renderGraphNameEmpty)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			if (ImGui::Button("Save"))
			{
				SaveToFile(renderGraphNameBuffer);
				done = true;
			}

			if (renderGraphNameEmpty)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			if (done)
			{
				ZERO_MEMORY(renderGraphNameBuffer, RENDER_GRAPH_NAME_BUFFER_LENGTH);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderLoadRenderGraphView()
	{
		ImGui::SetNextWindowSize(ImVec2(360, 400));
		if (ImGui::BeginPopupModal("Load Render Graph ##Popup"))
		{
			TArray<String> filesInDirectory = EnumerateFilesInDirectory("../Assets/RenderGraphs/", true);
			TArray<const char*> renderGraphFilesInDirectory;

			for (auto fileIt = filesInDirectory.begin(); fileIt != filesInDirectory.end(); fileIt++)
			{
				const String& filename = *fileIt;
				
				if (filename.find(".lrg") != String::npos)
				{
					renderGraphFilesInDirectory.push_back(filename.c_str());
				}
			}

			static int32 selectedIndex = 0;
			static bool loadSucceded = true;

			if (selectedIndex >= renderGraphFilesInDirectory.size()) selectedIndex = renderGraphFilesInDirectory.size() - 1;
			ImGui::ListBox("##Render Graph Files", &selectedIndex, renderGraphFilesInDirectory.data(), (int32)renderGraphFilesInDirectory.size());

			bool done = false;

			if (!loadSucceded)
			{
				ImGui::Text("Loading Failed!");
			}

			if (ImGui::Button("Close"))
			{
				done = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Load"))
			{
				loadSucceded = LoadFromFile(("../Assets/RenderGraphs/" + filesInDirectory[selectedIndex]), true);
				done = loadSucceded;
				m_ParsedGraphDirty = loadSucceded;
			}

			if (done)
			{
				selectedIndex	= 0;
				loadSucceded	= true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void RenderGraphEditor::RenderParsedRenderGraphView()
	{
		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		static String textBuffer0;
		static String textBuffer1;
		textBuffer0.resize(1024);
		textBuffer1.resize(1024);

		int32 currentAttributeIndex = INT32_MAX;

		static TArray<std::tuple<int32, int32, int32>> links;
		links.clear();
		links.reserve(m_ParsedRenderGraphStructure.PipelineStageDescriptions.size());

		float nodeXPos = 0.0f;
		float nodeXSpace = 350.0f;

		//Resource State Groups
		for (auto pipelineStageIt = m_ParsedRenderGraphStructure.PipelineStageDescriptions.begin(); pipelineStageIt != m_ParsedRenderGraphStructure.PipelineStageDescriptions.end(); pipelineStageIt++)
		{
			int32 distance	= std::distance(m_ParsedRenderGraphStructure.PipelineStageDescriptions.begin(), pipelineStageIt);
			int32 nodeIndex	= INT32_MAX - distance;
			const PipelineStageDesc* pPipelineStage = &(*pipelineStageIt);

			if (m_ParsedGraphRenderDirty)
			{
				imnodes::SetNodeGridSpacePos(nodeIndex, ImVec2(nodeXPos, 0.0f));
				nodeXPos += nodeXSpace;
			}

			imnodes::BeginNode(nodeIndex);

			if (pPipelineStage->Type == ERenderGraphPipelineStageType::RENDER)
			{
				const RenderStageDesc* pRenderStage = &m_ParsedRenderGraphStructure.RenderStageDescriptions[pPipelineStage->StageIndex];

				String renderStageType = RenderStageTypeToString(pRenderStage->Type);

				imnodes::BeginNodeTitleBar();
				ImGui::Text("Render Stage");
				ImGui::Text("%s : [%s]", pRenderStage->Name.c_str(), renderStageType.c_str());
				ImGui::Text("RS: %d PS: %d", pPipelineStage->StageIndex, distance);
				ImGui::Text("Weight: %d", pRenderStage->Weight);
				imnodes::EndNodeTitleBar();

				if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + " Child").c_str(), ImVec2(220.0f, 220.0f)))
				{
					for (auto resourceStateNameIt = pRenderStage->ResourceStates.begin(); resourceStateNameIt != pRenderStage->ResourceStates.end(); resourceStateNameIt++)
					{
						const RenderGraphResourceState* pResourceState = &(*resourceStateNameIt);
						auto resourceIt = FindResource(pResourceState->ResourceName);

						if (resourceIt != m_Resources.end())
						{
							textBuffer0 = "";
							textBuffer1 = "";

							textBuffer0 += resourceIt->Name.c_str();
							textBuffer1 += "Type: " + RenderGraphResourceTypeToString(resourceIt->Type);
							textBuffer1 += "\n";
							textBuffer1 += "Binding: " + BindingTypeToShortString(pResourceState->BindingType);
							textBuffer1 += "\n";
							textBuffer1 += "Sub Resource Count: " + std::to_string(resourceIt->SubResourceCount);

							if (resourceIt->Type == ERenderGraphResourceType::TEXTURE)
							{
								int32 textureFormatIndex = TextureFormatToFormatIndex(resourceIt->TextureParams.TextureFormat);

								textBuffer1 += "\n";
								textBuffer1 += "Texture Format: " + String(textureFormatIndex >= 0 ? TEXTURE_FORMAT_NAMES[textureFormatIndex] : "INVALID");
							}
							ImVec2 textSize = ImGui::CalcTextSize((textBuffer0 + textBuffer1 + "\n\n\n\n").c_str());

							if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + resourceIt->Name + " Child").c_str(), ImVec2(0.0f, textSize.y)))
							{
								ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), textBuffer0.c_str());
								ImGui::TextWrapped(textBuffer1.c_str());
							}

							ImGui::EndChild();
						}
					}
				}
				ImGui::EndChild();

				if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
				{
					ImGui::NewLine();
					ImGui::Text("RenderPass Transitions:");

					if (ImGui::BeginChild("##RenderPass Transitions", ImVec2(220.0f, 220.0f)))
					{
						for (auto resourceStateNameIt = pRenderStage->ResourceStates.begin(); resourceStateNameIt != pRenderStage->ResourceStates.end(); resourceStateNameIt++)
						{
							const RenderGraphResourceState* pResourceState = &(*resourceStateNameIt);

							if (pResourceState->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
							{
								auto resourceIt = FindResource(pResourceState->ResourceName);

								if (resourceIt != m_Resources.end())
								{
									textBuffer0 = "";
									textBuffer1 = "";

									textBuffer0 += resourceIt->Name.c_str();
									textBuffer1 += BindingTypeToShortString(pResourceState->AttachmentSynchronizations.PrevBindingType) + " -> " + BindingTypeToShortString(pResourceState->AttachmentSynchronizations.NextBindingType);
									ImVec2 textSize = ImGui::CalcTextSize((textBuffer0 + textBuffer1 + "\n\n\n").c_str());

									if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + resourceIt->Name + " Child").c_str(), ImVec2(0.0f, textSize.y)))
									{
										ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), textBuffer0.c_str());
										ImGui::TextWrapped(textBuffer1.c_str());
									}

									ImGui::EndChild();
								}
							}
						}
					}
					ImGui::EndChild();
				}

				imnodes::BeginInputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Input Button", ImGui::CalcTextSize("-"));
				imnodes::EndInputAttribute();

				ImGui::SameLine();

				imnodes::BeginOutputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Output Button", ImGui::CalcTextSize("-"));
				imnodes::EndOutputAttribute();
			}
			else if (pPipelineStage->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
			{
				const SynchronizationStageDesc* pSynchronizationStage = &m_ParsedRenderGraphStructure.SynchronizationStageDescriptions[pPipelineStage->StageIndex];

				imnodes::BeginNodeTitleBar();
				ImGui::Text("Synchronization Stage");
				ImGui::Text("SS: %d PS: %d", pPipelineStage->StageIndex, distance);
				imnodes::EndNodeTitleBar();

				if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + " Child").c_str(), ImVec2(220.0f, 220.0f)))
				{
					for (auto synchronizationIt = pSynchronizationStage->Synchronizations.begin(); synchronizationIt != pSynchronizationStage->Synchronizations.end(); synchronizationIt++)
					{
						const RenderGraphResourceSynchronizationDesc* pSynchronization = &(*synchronizationIt);
						auto resourceIt = FindResource(pSynchronization->ResourceName);

						if (resourceIt != m_Resources.end())
						{
							textBuffer0 = "";
							textBuffer1 = "";

							textBuffer0 += resourceIt->Name.c_str();
							textBuffer1 += "\n";
							textBuffer1 += CommandQueueToString(pSynchronization->PrevQueue) + " -> " + CommandQueueToString(pSynchronization->NextQueue);
							textBuffer1 += "\n";
							textBuffer1 += BindingTypeToShortString(pSynchronization->PrevBindingType) + " -> " + BindingTypeToShortString(pSynchronization->NextBindingType);
							ImVec2 textSize = ImGui::CalcTextSize((textBuffer0 + textBuffer1 + "\n\n\n\n").c_str());

							if (ImGui::BeginChild(("##" + std::to_string(pPipelineStage->StageIndex) + resourceIt->Name + " Child").c_str(), ImVec2(0.0f, textSize.y)))
							{
								ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), textBuffer0.c_str());
								ImGui::TextWrapped(textBuffer1.c_str());
							}

							ImGui::EndChild();
						}
					}
				}

				ImGui::EndChild();

				imnodes::BeginInputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Input Button", ImGui::CalcTextSize("-"));
				imnodes::EndInputAttribute();

				ImGui::SameLine();

				imnodes::BeginOutputAttribute(currentAttributeIndex--);
				ImGui::InvisibleButton("##Resouce State Invisible Output Button", ImGui::CalcTextSize("-"));
				imnodes::EndOutputAttribute();
			}

			if (pipelineStageIt != m_ParsedRenderGraphStructure.PipelineStageDescriptions.begin())
			{
				links.push_back(std::make_tuple(nodeIndex, currentAttributeIndex + 3, currentAttributeIndex + 2));
			}

			imnodes::EndNode();
		}

		for (auto linkIt = links.begin(); linkIt != links.end(); linkIt++)
		{
			imnodes::Link(std::get<0>(*linkIt), std::get<1>(*linkIt), std::get<2>(*linkIt));
		}

		imnodes::EndNodeEditor();

		m_ParsedGraphRenderDirty = false;
	}

	void RenderGraphEditor::RenderShaderBoxes(EditorRenderStageDesc* pRenderStage)
	{
		if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
		{
			if (pRenderStage->Graphics.Shaders.VertexShaderName.size()	== 0 &&
				pRenderStage->Graphics.Shaders.GeometryShaderName.size() == 0 &&
				pRenderStage->Graphics.Shaders.HullShaderName.size()		== 0 &&
				pRenderStage->Graphics.Shaders.DomainShaderName.size()	== 0)
			{
				ImGui::PushID("##Task Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.TaskShaderName.size() == 0 ? "Task Shader" : pRenderStage->Graphics.Shaders.TaskShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.TaskShaderName);
				ImGui::PopID();
				
				ImGui::PushID("##Mesh Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.MeshShaderName.size() == 0 ? "Mesh Shader" : pRenderStage->Graphics.Shaders.MeshShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.MeshShaderName);
				ImGui::PopID();
			}

			if (pRenderStage->Graphics.Shaders.TaskShaderName.size()	== 0 &&
				pRenderStage->Graphics.Shaders.MeshShaderName.size() == 0)
			{
				ImGui::PushID("##Vertex Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.VertexShaderName.size() == 0 ? "Vertex Shader" : pRenderStage->Graphics.Shaders.VertexShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.VertexShaderName);
				ImGui::PopID();
				
				ImGui::PushID("##Geometry Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.GeometryShaderName.size() == 0 ? "Geometry Shader" : pRenderStage->Graphics.Shaders.GeometryShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.GeometryShaderName);
				ImGui::PopID();

				ImGui::PushID("##Hull Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.HullShaderName.size() == 0 ? "Hull Shader" : pRenderStage->Graphics.Shaders.HullShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.HullShaderName);
				ImGui::PopID();

				ImGui::PushID("##Domain Shader ID");
				ImGui::Button(pRenderStage->Graphics.Shaders.DomainShaderName.size() == 0 ? "Domain Shader" : pRenderStage->Graphics.Shaders.DomainShaderName.c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.DomainShaderName);
				ImGui::PopID();
			}

			ImGui::PushID("##Pixel Shader ID");
			ImGui::Button(pRenderStage->Graphics.Shaders.PixelShaderName.size() == 0 ? "Pixel Shader" : pRenderStage->Graphics.Shaders.PixelShaderName.c_str());
			RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.PixelShaderName);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::COMPUTE)
		{
			ImGui::PushID("##Compute Shader ID");
			ImGui::Button(pRenderStage->Compute.ShaderName.size() == 0 ? "Shader" : pRenderStage->Compute.ShaderName.c_str());
			RenderShaderBoxCommon(&pRenderStage->Compute.ShaderName);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::RAY_TRACING)
		{
			ImGui::PushID("##Raygen Shader ID");
			ImGui::Button(pRenderStage->RayTracing.Shaders.RaygenShaderName.size() == 0 ? "Raygen Shader" : pRenderStage->RayTracing.Shaders.RaygenShaderName.c_str());
			RenderShaderBoxCommon(&pRenderStage->RayTracing.Shaders.RaygenShaderName);
			ImGui::PopID();

			uint32 missBoxesCount = glm::min(pRenderStage->RayTracing.Shaders.MissShaderCount + 1, MAX_MISS_SHADER_COUNT);
			for (uint32 m = 0; m < missBoxesCount; m++)
			{
				bool added = false;
				bool removed = false;

				ImGui::PushID(m);
				ImGui::Button(pRenderStage->RayTracing.Shaders.pMissShaderNames[m].size() == 0 ? "Miss Shader" : pRenderStage->RayTracing.Shaders.pMissShaderNames[m].c_str());
				RenderShaderBoxCommon(&(pRenderStage->RayTracing.Shaders.pMissShaderNames[m]), &added, &removed);
				ImGui::PopID();

				if (added) 
					pRenderStage->RayTracing.Shaders.MissShaderCount++;

				if (removed)
				{
					for (uint32 m2 = m; m2 < missBoxesCount - 1; m2++)
					{
						pRenderStage->RayTracing.Shaders.pMissShaderNames[m2] = pRenderStage->RayTracing.Shaders.pMissShaderNames[m2 + 1];
					}

					pRenderStage->RayTracing.Shaders.MissShaderCount--;
					missBoxesCount--;
				}
			}

			uint32 closestHitBoxesCount = glm::min(pRenderStage->RayTracing.Shaders.ClosestHitShaderCount + 1, MAX_CLOSEST_HIT_SHADER_COUNT);
			for (uint32 ch = 0; ch < closestHitBoxesCount; ch++)
			{
				bool added = false;
				bool removed = false;

				ImGui::PushID(ch);
				ImGui::Button(pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch].size() == 0 ? "Closest Hit Shader" : pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch].c_str());
				RenderShaderBoxCommon(&pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch], &added, &removed);
				ImGui::PopID();

				if (added)
					pRenderStage->RayTracing.Shaders.ClosestHitShaderCount++;

				if (removed)
				{
					for (uint32 ch2 = ch; ch2 < missBoxesCount - 1; ch2++)
					{
						pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch2] = pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch2 + 1];
					}

					pRenderStage->RayTracing.Shaders.ClosestHitShaderCount--;
					closestHitBoxesCount--;
				}
			}
		}
	}

	void RenderGraphEditor::RenderShaderBoxCommon(String* pTarget, bool* pAdded, bool* pRemoved)
	{
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("SHADER");

			if (pPayload != nullptr)
			{
				String* pShaderName = *reinterpret_cast<String**>(pPayload->Data);
				if (pAdded != nullptr && pTarget->size() == 0) (*pAdded) = true;
				(*pTarget) = *pShaderName;
			}

			ImGui::EndDragDropTarget();
		}

		if (pTarget->size() > 0)
		{
			ImGui::SameLine();

			if (ImGui::Button("-"))
			{
				(*pTarget) = "";
				if (pRemoved != nullptr) (*pRemoved) = true;
			}
		}
	}

	TArray<RenderGraphResourceDesc>::iterator RenderGraphEditor::FindResource(const String& name)
	{
		for (auto resourceIt = m_Resources.begin(); resourceIt != m_Resources.end(); resourceIt++)
		{
			if (resourceIt->Name == name)
				return resourceIt;
		}

		return m_Resources.end();
	}

	EditorResourceStateIdent RenderGraphEditor::CreateResourceState(const String& resourceName, const String& renderStageName, bool removable, ERenderGraphResourceBindingType bindingType)
	{
		if (bindingType == ERenderGraphResourceBindingType::NONE)
		{
			auto resourceIt = FindResource(resourceName);

			if (resourceIt != m_Resources.end())
			{
				switch (resourceIt->Type)
				{
					case ERenderGraphResourceType::TEXTURE:
					{
						bindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;
						break;
					}
					case ERenderGraphResourceType::BUFFER:
					{
						bindingType = ERenderGraphResourceBindingType::CONSTANT_BUFFER;
						break;
					}
					case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
					{
						bindingType = ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE;
						break;
					}
				}
			}
		}

		EditorRenderGraphResourceState resourceState = {};
		resourceState.ResourceName		= resourceName;
		resourceState.RenderStageName	= renderStageName;
		resourceState.Removable			= removable;
		resourceState.BindingType		= bindingType;

		int32 attributeIndex = s_NextAttributeID;
		s_NextAttributeID += 2;

		m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2]			= resourceState;
		return { resourceName, attributeIndex };
	}

	bool RenderGraphEditor::CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex)
	{
		int32 src = *pSrcAttributeIndex;
		int32 dst = *pDstAttributeIndex;
		
		if (src % 2 == 1 && dst % 2 == 0)
		{
			if (dst + 1 == src)
				return false;
			
			return true;
		}
		else if (src % 2 == 0 && dst % 2 == 1)
		{
			if (src + 1 == dst)
				return false;

			(*pSrcAttributeIndex) = dst;
			(*pDstAttributeIndex) = src;
			return true;
		}

		return false;
	}

	void RenderGraphEditor::RemoveResourceStateFrom(const String& name, EditorResourceStateGroup* pResourceStateGroup)
	{
		auto resourceStateIndexIt = pResourceStateGroup->FindResourceStateIdent(name);

		if (resourceStateIndexIt != pResourceStateGroup->ResourceStateIdents.end())
		{
			int32 attributeIndex = resourceStateIndexIt->AttributeIndex;
			auto resourceStateIt = m_ResourceStatesByHalfAttributeIndex.find(attributeIndex / 2);
			EditorRenderGraphResourceState* pResourceState = &resourceStateIt->second;

			DestroyLink(pResourceState->InputLinkIndex);

			//Copy so that DestroyLink wont delete from set we're iterating through
			TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
			for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
			{
				int32 linkToBeDestroyedIndex = *outputLinkIt;
				DestroyLink(linkToBeDestroyedIndex);
			}

			m_ResourceStatesByHalfAttributeIndex.erase(resourceStateIt);
			pResourceStateGroup->ResourceStateIdents.erase(resourceStateIndexIt);
		}
	}

	void RenderGraphEditor::RemoveResourceStateFrom(const String& name, EditorRenderStageDesc* pRenderStageDesc)
	{
		auto resourceStateIndexIt = pRenderStageDesc->FindResourceStateIdent(name);

		if (resourceStateIndexIt != pRenderStageDesc->ResourceStateIdents.end())
		{
			int32 attributeIndex = resourceStateIndexIt->AttributeIndex;
			auto resourceStateIt = m_ResourceStatesByHalfAttributeIndex.find(attributeIndex / 2);
			EditorRenderGraphResourceState* pResourceState = &resourceStateIt->second;

			DestroyLink(pResourceState->InputLinkIndex);

			//Copy so that DestroyLink wont delete from set we're iterating through
			TSet<int32> outputLinkIndices = pResourceState->OutputLinkIndices;
			for (auto outputLinkIt = outputLinkIndices.begin(); outputLinkIt != outputLinkIndices.end(); outputLinkIt++)
			{
				int32 linkToBeDestroyedIndex = *outputLinkIt;
				DestroyLink(linkToBeDestroyedIndex);
			}

			m_ResourceStatesByHalfAttributeIndex.erase(resourceStateIt);
			pRenderStageDesc->ResourceStateIdents.erase(resourceStateIndexIt);
		}
	}

	void RenderGraphEditor::DestroyLink(int32 linkIndex)
	{
		if (linkIndex >= 0)
		{
			EditorRenderGraphResourceLink* pLinkToBeDestroyed = &m_ResourceStateLinksByLinkIndex[linkIndex];

			EditorRenderGraphResourceState* pSrcResource = &m_ResourceStatesByHalfAttributeIndex[pLinkToBeDestroyed->SrcAttributeIndex / 2];
			EditorRenderGraphResourceState* pDstResource = &m_ResourceStatesByHalfAttributeIndex[pLinkToBeDestroyed->DstAttributeIndex / 2];

			m_ResourceStateLinksByLinkIndex.erase(linkIndex);

			pDstResource->InputLinkIndex = -1;
			pSrcResource->OutputLinkIndices.erase(linkIndex);
			m_ParsedGraphDirty = true;
		}
	}

	void RenderGraphEditor::PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pRenderStage, pResourceState, targetAttributeIndex))
		{
			imnodes::PushColorStyle(imnodes::ColorStyle_Pin,		HOVERED_COLOR);
			imnodes::PushColorStyle(imnodes::ColorStyle_PinHovered, HOVERED_COLOR);
		}
	}

	void RenderGraphEditor::PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pRenderStage, pResourceState, targetAttributeIndex))
		{
			imnodes::PopColorStyle();
			imnodes::PopColorStyle();
		}
	}

	bool RenderGraphEditor::CustomPinColorNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (m_StartedLinkInfo.LinkStarted)
		{
			if (pResourceState == nullptr)
			{
				//Started on Output Pin and target is RENDER_STAGE_INPUT Pin
				if (!m_StartedLinkInfo.LinkStartedOnInputPin && pinType == EEditorPinType::RENDER_STAGE_INPUT)
				{
					return true;
				}
			}
			else if (m_StartedLinkInfo.LinkStartedOnResource == pResourceState->ResourceName || m_StartedLinkInfo.LinkStartedOnResource.size() == 0)
			{
				if (!m_StartedLinkInfo.LinkStartedOnInputPin)
				{
					if (pinType == EEditorPinType::INPUT && (targetAttributeIndex + 1) != m_StartedLinkInfo.LinkStartAttributeID)
					{
						return true;
					}
				}
				else
				{
					if (pinType == EEditorPinType::OUTPUT && (m_StartedLinkInfo.LinkStartAttributeID + 1) != targetAttributeIndex)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void RenderGraphEditor::CalculateResourceStateBindingTypes(const EditorRenderStageDesc* pRenderStage, const RenderGraphResourceDesc* pResource, const EditorRenderGraphResourceState* pResourceState, TArray<ERenderGraphResourceBindingType>& bindingTypes, TArray<const char*>& bindingTypeNames)
	{
		bool read	= pResourceState->InputLinkIndex != -1;
		bool write	= pResourceState->OutputLinkIndices.size() > 0;

		switch (pResource->Type)
		{
			case ERenderGraphResourceType::TEXTURE:
			{
				if (pRenderStage->OverrideRecommendedBindingType)
				{
					if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
					{
						if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
						{
							bindingTypes.push_back(ERenderGraphResourceBindingType::ATTACHMENT);
							bindingTypeNames.push_back("ATTACHMENT");
						}
					}

					bindingTypes.push_back(ERenderGraphResourceBindingType::COMBINED_SAMPLER);
					bindingTypeNames.push_back("COMBINED SAMPLER");
					
					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.push_back("UNORDERED ACCESS R");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS W");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE TEXTURE
						if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.push_back(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.push_back("ATTACHMENT");
							}
						}

						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ TEXTURE

						//If the texture is of Depth/Stencil format we allow Attachment as binding type even if it is discovered as in read mode
						if (pResource->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
						{
							if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
							{
								if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
								{
									bindingTypes.push_back(ERenderGraphResourceBindingType::ATTACHMENT);
									bindingTypeNames.push_back("ATTACHMENT");
								}
							}
						}

						bindingTypes.push_back(ERenderGraphResourceBindingType::COMBINED_SAMPLER);
						bindingTypeNames.push_back("COMBINED SAMPLER");

						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.push_back("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE TEXTURE

						if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.push_back(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.push_back("ATTACHMENT");
							}
						}

						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS W");
					}
					else if (pResource->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
					{
						//If the texture is of Depth/Stencil format we allow Attachment as binding type even if it is discovered as in read mode

						if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.push_back(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.push_back("ATTACHMENT");
							}
						}
					}
				}
				break;
			}
			case ERenderGraphResourceType::BUFFER:
			{
				if (pRenderStage->OverrideRecommendedBindingType)
				{
					bindingTypes.push_back(ERenderGraphResourceBindingType::CONSTANT_BUFFER);
					bindingTypeNames.push_back("CONSTANT BUFFER");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.push_back("UNORDERED ACCESS R");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS W");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE BUFFER
						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ BUFFER
						bindingTypes.push_back(ERenderGraphResourceBindingType::CONSTANT_BUFFER);
						bindingTypeNames.push_back("CONSTANT BUFFER");

						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.push_back("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE BUFFER
						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS W");
					}
				}

				break;
			}
			case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
			{
				if (pRenderStage->OverrideRecommendedBindingType)
				{
					bindingTypes.push_back(ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE);
					bindingTypeNames.push_back("ACCELERATION STRUCTURE");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.push_back("UNORDERED ACCESS R");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS W");

					bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.push_back("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE ACCELERATION_STRUCTURE
						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ ACCELERATION_STRUCTURE
						bindingTypes.push_back(ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE);
						bindingTypeNames.push_back("ACCELERATION STRUCTURE");

						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.push_back("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE ACCELERATION_STRUCTURE
						bindingTypes.push_back(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.push_back("UNORDERED ACCESS W");
					}
				}

				break;
			}
		}
	}

	bool RenderGraphEditor::SaveToFile(const String& renderGraphName)
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
				for (const RenderGraphResourceDesc& resource : m_Resources)
				{
					writer.StartObject();
					{
						writer.String("name");
						writer.String(resource.Name.c_str());

						writer.String("type");
						writer.String(RenderGraphResourceTypeToString(resource.Type).c_str());

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
									writer.String(TextureFormatToString(resource.TextureParams.TextureFormat).c_str());

									writer.String("is_of_array_type");
									writer.Bool(resource.TextureParams.IsOfArrayType);

									if (!resource.External && resource.Name != RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
									{
										writer.String("x_dim_type");
										writer.String(RenderGraphDimensionTypeToString(resource.TextureParams.XDimType).c_str());

										writer.String("y_dim_type");
										writer.String(RenderGraphDimensionTypeToString(resource.TextureParams.YDimType).c_str());

										writer.String("x_dim_var");
										writer.Double(resource.TextureParams.XDimVariable);

										writer.String("y_dim_var");
										writer.Double(resource.TextureParams.YDimVariable);

										writer.String("sample_count");
										writer.Int(resource.TextureParams.SampleCount);

										writer.String("miplevel_count");
										writer.Int(resource.TextureParams.MiplevelCount);

										writer.String("sampler_type");
										writer.String(RenderGraphSamplerTypeToString(resource.TextureParams.SamplerType).c_str());

										writer.String("memory_type");
										writer.String(MemoryTypeToString(resource.MemoryType).c_str());
									}

									break;
								}
								case ERenderGraphResourceType::BUFFER:
								{
									if (!resource.External)
									{
										writer.String("size_type");
										writer.String(RenderGraphDimensionTypeToString(resource.BufferParams.SizeType).c_str());

										writer.String("size");
										writer.Int(resource.BufferParams.Size);

										writer.String("memory_type");
										writer.String(MemoryTypeToString(resource.MemoryType).c_str());
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
				for (auto resourceStateGroupIt = m_ResourceStateGroups.begin(); resourceStateGroupIt != m_ResourceStateGroups.end(); resourceStateGroupIt++)
				{
					EditorResourceStateGroup* pResourceStateGroup = &(*resourceStateGroupIt);

					writer.StartObject();
					{
						writer.String("name");
						writer.String(pResourceStateGroup->Name.c_str());

						writer.String("input_node_index");
						writer.Int(pResourceStateGroup->InputNodeIndex);

						writer.String("output_node_index");
						writer.Int(pResourceStateGroup->OutputNodeIndex);

						writer.String("resource_states");
						writer.StartArray();
						{
							for (const EditorResourceStateIdent& resourceStateIdent : pResourceStateGroup->ResourceStateIdents)
							{
								int32 attributeIndex = resourceStateIdent.AttributeIndex;
								const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

								writer.StartObject();
								{
									writer.String("name");
									writer.String(pResourceState->ResourceName.c_str());

									writer.String("render_stage_name");
									writer.String(pResourceState->RenderStageName.c_str());

									writer.String("removable");
									writer.Bool(pResourceState->Removable);

									writer.String("binding_type");
									writer.String(BindingTypeToString(pResourceState->BindingType).c_str());

									writer.String("attribute_index");
									writer.Int(attributeIndex);

									writer.String("input_link_index");
									writer.Int(pResourceState->InputLinkIndex);

									writer.String("output_link_indices");
									writer.StartArray();
									{
										for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
										{
											int32 outputLinkIndex = *outputLinkIt;

											writer.Int(outputLinkIndex);
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
				writer.String(m_FinalOutput.Name.c_str());

				writer.String("node_index");
				writer.Int(m_FinalOutput.NodeIndex);

				writer.String("back_buffer_state");
				writer.StartObject();
				{
					int32 attributeIndex = m_FinalOutput.BackBufferAttributeIndex;
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

					writer.String("name");
					writer.String(pResourceState->ResourceName.c_str());

					writer.String("render_stage_name");
					writer.String(pResourceState->RenderStageName.c_str());

					writer.String("removable");
					writer.Bool(pResourceState->Removable);

					writer.String("binding_type");
					writer.String(BindingTypeToString(pResourceState->BindingType).c_str());

					writer.String("attribute_index");
					writer.Int(attributeIndex);

					writer.String("input_link_index");
					writer.Int(pResourceState->InputLinkIndex);

					writer.String("output_link_indices");
					writer.StartArray();
					{
						for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
						{
							int32 outputLinkIndex = *outputLinkIt;

							writer.Int(outputLinkIndex);
						}
					}
					writer.EndArray();
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
				for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
				{
					EditorRenderStageDesc* pRenderStage = &renderStageIt->second;

					writer.StartObject();
					{
						writer.String("name");
						writer.String(pRenderStage->Name.c_str());

						writer.String("node_index");
						writer.Int(pRenderStage->NodeIndex);

						writer.String("input_attribute_index");
						writer.Int(pRenderStage->InputAttributeIndex);

						writer.String("type");
						writer.String(RenderStageTypeToString(pRenderStage->Type).c_str());

						writer.String("custom_renderer");
						writer.Bool(pRenderStage->CustomRenderer);

						writer.String("x_dim_type");
						writer.String(RenderGraphDimensionTypeToString(pRenderStage->Parameters.XDimType).c_str());

						writer.String("y_dim_type");
						writer.String(RenderGraphDimensionTypeToString(pRenderStage->Parameters.YDimType).c_str());

						writer.String("z_dim_type");
						writer.String(RenderGraphDimensionTypeToString(pRenderStage->Parameters.ZDimType).c_str());

						writer.String("x_dim_var");
						writer.Double(pRenderStage->Parameters.XDimVariable);

						writer.String("y_dim_var");
						writer.Double(pRenderStage->Parameters.YDimVariable);

						writer.String("z_dim_var");
						writer.Double(pRenderStage->Parameters.ZDimVariable);

						if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
						{
							writer.String("draw_params");
							writer.StartObject();
							{
								writer.String("draw_type");
								writer.String(RenderStageDrawTypeToString(pRenderStage->Graphics.DrawType).c_str());

								//Index Buffer Draw Resource
								{
									int32 attributeIndex = pRenderStage->Graphics.IndexBufferAttributeIndex;
									EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

									writer.String("index_buffer");
									writer.StartObject();
									{
										writer.String("name");
										writer.String(pResourceState->ResourceName.c_str());

										writer.String("render_stage_name");
										writer.String(pResourceState->RenderStageName.c_str());

										writer.String("removable");
										writer.Bool(pResourceState->Removable);

										writer.String("binding_type");
										writer.String(BindingTypeToString(pResourceState->BindingType).c_str());

										writer.String("attribute_index");
										writer.Int(attributeIndex);

										writer.String("input_link_index");
										writer.Int(pResourceState->InputLinkIndex);

										writer.String("output_link_indices");
										writer.StartArray();
										{
											for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
											{
												int32 outputLinkIndex = *outputLinkIt;

												writer.Int(outputLinkIndex);
											}
										}
										writer.EndArray();
									}
									writer.EndObject();
								}

								//Indirect Args Buffer Draw Resource
								{
									int32 attributeIndex = pRenderStage->Graphics.IndirectArgsBufferAttributeIndex;
									EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

									writer.String("indirect_args_buffer");
									writer.StartObject();
									{
										writer.String("name");
										writer.String(pResourceState->ResourceName.c_str());

										writer.String("render_stage_name");
										writer.String(pResourceState->RenderStageName.c_str());

										writer.String("removable");
										writer.Bool(pResourceState->Removable);

										writer.String("binding_type");
										writer.String(BindingTypeToString(pResourceState->BindingType).c_str());

										writer.String("attribute_index");
										writer.Int(attributeIndex);

										writer.String("input_link_index");
										writer.Int(pResourceState->InputLinkIndex);

										writer.String("output_link_indices");
										writer.StartArray();
										{
											for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
											{
												int32 outputLinkIndex = *outputLinkIt;

												writer.Int(outputLinkIndex);
											}
										}
										writer.EndArray();
									}
									writer.EndObject();
								}
							}
							writer.EndObject();

							writer.String("depth_test_enabled");
							writer.Bool(pRenderStage->Graphics.DepthTestEnabled);
						}

						writer.String("shaders");
						writer.StartObject();
						{
							if (pRenderStage->Type == EPipelineStateType::GRAPHICS)
							{
								writer.String("task_shader");
								writer.String(pRenderStage->Graphics.Shaders.TaskShaderName.c_str());

								writer.String("mesh_shader");
								writer.String(pRenderStage->Graphics.Shaders.MeshShaderName.c_str());

								writer.String("vertex_shader");
								writer.String(pRenderStage->Graphics.Shaders.VertexShaderName.c_str());

								writer.String("geometry_shader");
								writer.String(pRenderStage->Graphics.Shaders.GeometryShaderName.c_str());

								writer.String("hull_shader");
								writer.String(pRenderStage->Graphics.Shaders.HullShaderName.c_str());

								writer.String("domain_shader");
								writer.String(pRenderStage->Graphics.Shaders.DomainShaderName.c_str());

								writer.String("pixel_shader");
								writer.String(pRenderStage->Graphics.Shaders.PixelShaderName.c_str());
							}
							else if (pRenderStage->Type == EPipelineStateType::COMPUTE)
							{
								writer.String("shader");
								writer.String(pRenderStage->Compute.ShaderName.c_str());
							}
							else if (pRenderStage->Type == EPipelineStateType::RAY_TRACING)
							{
								writer.String("raygen_shader");
								writer.String(pRenderStage->RayTracing.Shaders.RaygenShaderName.c_str());

								writer.String("miss_shaders");
								writer.StartArray();
								for (uint32 m = 0; m < pRenderStage->RayTracing.Shaders.MissShaderCount; m++)
								{
									writer.String(pRenderStage->RayTracing.Shaders.pMissShaderNames[m].c_str());
								}
								writer.EndArray();

								writer.String("closest_hit_shaders");
								writer.StartArray();
								for (uint32 ch = 0; ch < pRenderStage->RayTracing.Shaders.ClosestHitShaderCount; ch++)
								{
									writer.String(pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch].c_str());
								}
								writer.EndArray();
							}
						}
						writer.EndObject();

						writer.String("resource_states");
						writer.StartArray();
						{
							for (const EditorResourceStateIdent& resourceStateIdent : pRenderStage->ResourceStateIdents)
							{
								int32 attributeIndex = resourceStateIdent.AttributeIndex;
								EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2];

								writer.StartObject();
								{
									writer.String("name");
									writer.String(pResourceState->ResourceName.c_str());

									writer.String("render_stage_name");
									writer.String(pResourceState->RenderStageName.c_str());

									writer.String("removable");
									writer.Bool(pResourceState->Removable);

									writer.String("binding_type");
									writer.String(BindingTypeToString(pResourceState->BindingType).c_str());

									writer.String("attribute_index");
									writer.Int(attributeIndex);

									writer.String("input_link_index");
									writer.Int(pResourceState->InputLinkIndex);

									writer.String("output_link_indices");
									writer.StartArray();
									{
										for (auto outputLinkIt = pResourceState->OutputLinkIndices.begin(); outputLinkIt != pResourceState->OutputLinkIndices.end(); outputLinkIt++)
										{
											int32 outputLinkIndex = *outputLinkIt;

											writer.Int(outputLinkIndex);
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
				}
			}
			writer.EndArray();
		}


		//Links
		{
			writer.String("links");
			writer.StartArray();
			{
				for (auto linkIt = m_ResourceStateLinksByLinkIndex.begin(); linkIt != m_ResourceStateLinksByLinkIndex.end(); linkIt++)
				{
					EditorRenderGraphResourceLink* pLink = &linkIt->second;

					writer.StartObject();
					{
						writer.String("link_index");
						writer.Int(pLink->LinkIndex);

						writer.String("src_attribute_index");
						writer.Int(pLink->SrcAttributeIndex);

						writer.String("dst_attribute_index");
						writer.Int(pLink->DstAttributeIndex);
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

	bool RenderGraphEditor::LoadFromFile(const String& filepath, bool generateImGuiStage)
	{
		using namespace rapidjson;

		FILE* pFile = fopen(filepath.c_str(), "r");

		if (pFile == nullptr)
			return false;

		char readBuffer[65536];
		FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		Document d;
		d.ParseStream(inputStream);

		int32 largestNodeID			= 0;
		int32 largestAttributeID	= 0;
		int32 largestLinkID			= 0;

		std::vector<EditorResourceStateGroup>				loadedResourceStateGroups;
		EditorFinalOutput									loadedFinalOutput		= {};

		TArray<RenderGraphResourceDesc>						loadedResources;
		THashTable<int32, String>							loadedRenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStageDesc>			loadedRenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	loadedResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	loadedResourceStateLinks;

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
								resource.TextureParams.TextureFormat	= TextureFormatFromString(resourceTypeParamsObject["texture_format"].GetString());
								resource.TextureParams.IsOfArrayType	= resourceTypeParamsObject["is_of_array_type"].GetBool();

								if (!resource.External && resource.Name != RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
								{
									resource.TextureParams.XDimType			= RenderGraphDimensionTypeFromString(resourceTypeParamsObject["x_dim_type"].GetString());
									resource.TextureParams.YDimType			= RenderGraphDimensionTypeFromString(resourceTypeParamsObject["y_dim_type"].GetString());
									resource.TextureParams.XDimVariable		= resourceTypeParamsObject["x_dim_var"].GetFloat();
									resource.TextureParams.YDimVariable		= resourceTypeParamsObject["y_dim_var"].GetFloat();
									resource.TextureParams.SampleCount		= resourceTypeParamsObject["sample_count"].GetInt();
									resource.TextureParams.MiplevelCount	= resourceTypeParamsObject["miplevel_count"].GetInt();
									resource.TextureParams.SamplerType		= RenderGraphSamplerTypeFromString(resourceTypeParamsObject["sampler_type"].GetString());
									resource.MemoryType						= MemoryTypeFromString(resourceTypeParamsObject["memory_type"].GetString());
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
					
					loadedResources.push_back(resource);
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
					resourceStateGroup.InputNodeIndex	= resourceStateGroupObject["input_node_index"].GetInt();
					resourceStateGroup.OutputNodeIndex	= resourceStateGroupObject["output_node_index"].GetInt();

					GenericArray resourceStateArray = resourceStateGroupObject["resource_states"].GetArray();

					for (uint32 r = 0; r < resourceStateArray.Size(); r++)
					{
						GenericObject resourceStateObject = resourceStateArray[r].GetObject();

						String resourceName		= resourceStateObject["name"].GetString();
						int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

						EditorRenderGraphResourceState resourceState = {};
						resourceState.ResourceName		= resourceName;
						resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
						resourceState.Removable			= resourceStateObject["removable"].GetBool();
						resourceState.BindingType		= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());
						resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();

						GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

						for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
						{
							resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
						}

						loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
						resourceStateGroup.ResourceStateIdents.push_back({ resourceName, attributeIndex });

						if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
					}

					loadedResourceStateGroups.push_back(resourceStateGroup);

					if (resourceStateGroup.InputNodeIndex > largestNodeID) largestNodeID = resourceStateGroup.InputNodeIndex;
					if (resourceStateGroup.OutputNodeIndex > largestNodeID) largestNodeID = resourceStateGroup.OutputNodeIndex;
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

				loadedFinalOutput.Name		= finalOutputStageObject["name"].GetString();
				loadedFinalOutput.NodeIndex = finalOutputStageObject["node_index"].GetInt();

				GenericObject resourceStateObject = finalOutputStageObject["back_buffer_state"].GetObject();

				String resourceName		= resourceStateObject["name"].GetString();
				int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

				EditorRenderGraphResourceState resourceState = {};
				resourceState.ResourceName		= resourceName;
				resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
				resourceState.Removable			= resourceStateObject["removable"].GetBool();
				resourceState.BindingType		= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());
				resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();

				GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

				for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
				{
					resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
				}

				loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
				loadedFinalOutput.BackBufferAttributeIndex = attributeIndex;

				if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;

				if (loadedFinalOutput.NodeIndex > largestNodeID) largestNodeID = loadedFinalOutput.NodeIndex;
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
					renderStage.NodeIndex			= renderStageObject["node_index"].GetInt();
					renderStage.InputAttributeIndex = renderStageObject["input_attribute_index"].GetInt();
					renderStage.Type				= RenderStageTypeFromString(renderStageObject["type"].GetString());
					renderStage.CustomRenderer		= renderStageObject["custom_renderer"].GetBool();

					renderStage.Parameters.XDimType			= RenderGraphDimensionTypeFromString(renderStageObject["x_dim_type"].GetString());
					renderStage.Parameters.YDimType			= RenderGraphDimensionTypeFromString(renderStageObject["y_dim_type"].GetString());
					renderStage.Parameters.ZDimType			= RenderGraphDimensionTypeFromString(renderStageObject["z_dim_type"].GetString());

					renderStage.Parameters.XDimVariable		= renderStageObject["x_dim_var"].GetDouble();
					renderStage.Parameters.YDimVariable		= renderStageObject["y_dim_var"].GetDouble();
					renderStage.Parameters.ZDimVariable		= renderStageObject["z_dim_var"].GetDouble();

					GenericObject shadersObject		= renderStageObject["shaders"].GetObject();
					GenericArray resourceStateArray = renderStageObject["resource_states"].GetArray();

					if (renderStage.Type == EPipelineStateType::GRAPHICS)
					{
						GenericObject drawParamsObject	= renderStageObject["draw_params"].GetObject();

						renderStage.Graphics.DrawType = RenderStageDrawTypeFromString(drawParamsObject["draw_type"].GetString());

						//Index Buffer
						{
							GenericObject resourceStateObject = drawParamsObject["index_buffer"].GetObject();

							String resourceName		= resourceStateObject["name"].GetString();
							int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

							EditorRenderGraphResourceState resourceState = {};
							resourceState.ResourceName		= resourceName;
							resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
							resourceState.Removable			= resourceStateObject["removable"].GetBool();
							resourceState.BindingType		= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());
							resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();
						
							GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

							for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
							{
								resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
							}

							loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
							renderStage.Graphics.IndexBufferAttributeIndex = attributeIndex;

							if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
						}

						//Indirect Args Buffer
						{
							GenericObject resourceStateObject = drawParamsObject["indirect_args_buffer"].GetObject();

							String resourceName		= resourceStateObject["name"].GetString();
							int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

							EditorRenderGraphResourceState resourceState = {};
							resourceState.ResourceName		= resourceName;
							resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
							resourceState.Removable			= resourceStateObject["removable"].GetBool();
							resourceState.BindingType		= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());
							resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();
						
							GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

							for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
							{
								resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
							}

							loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
							renderStage.Graphics.IndirectArgsBufferAttributeIndex = attributeIndex;

							if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
						}

						renderStage.Graphics.DepthTestEnabled			= renderStageObject["depth_test_enabled"].GetBool();

						renderStage.Graphics.Shaders.TaskShaderName		= shadersObject["task_shader"].GetString();
						renderStage.Graphics.Shaders.MeshShaderName		= shadersObject["mesh_shader"].GetString();
						renderStage.Graphics.Shaders.VertexShaderName	= shadersObject["vertex_shader"].GetString();
						renderStage.Graphics.Shaders.GeometryShaderName	= shadersObject["geometry_shader"].GetString();
						renderStage.Graphics.Shaders.HullShaderName		= shadersObject["hull_shader"].GetString();
						renderStage.Graphics.Shaders.DomainShaderName	= shadersObject["domain_shader"].GetString();
						renderStage.Graphics.Shaders.PixelShaderName	= shadersObject["pixel_shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::COMPUTE)
					{
						renderStage.Compute.ShaderName			= shadersObject["shader"].GetString();
					}
					else if (renderStage.Type == EPipelineStateType::RAY_TRACING)
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
						int32 attributeIndex	= resourceStateObject["attribute_index"].GetInt();

						EditorRenderGraphResourceState resourceState = {};
						resourceState.ResourceName		= resourceName;
						resourceState.RenderStageName	= resourceStateObject["render_stage_name"].GetString();
						resourceState.Removable			= resourceStateObject["removable"].GetBool();
						resourceState.BindingType		= ResourceStateBindingTypeFromString(resourceStateObject["binding_type"].GetString());
						resourceState.InputLinkIndex	= resourceStateObject["input_link_index"].GetInt();
						
						GenericArray outputLinkIndicesArray = resourceStateObject["output_link_indices"].GetArray();

						for (uint32 ol = 0; ol < outputLinkIndicesArray.Size(); ol++)
						{
							resourceState.OutputLinkIndices.insert(outputLinkIndicesArray[ol].GetInt());
						}

						loadedResourceStatesByHalfAttributeIndex[attributeIndex / 2] = resourceState;
						renderStage.ResourceStateIdents.push_back({ resourceName, attributeIndex });

						if (attributeIndex + 1 > largestAttributeID) largestAttributeID = attributeIndex + 1;
					}

					loadedRenderStagesByName[renderStage.Name] = renderStage;
					loadedRenderStageNameByInputAttributeIndex[renderStage.InputAttributeIndex] = renderStage.Name;

					if (renderStage.NodeIndex > largestNodeID) largestNodeID = renderStage.NodeIndex;
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

		//Load Resouce State Links
		if (d.HasMember("links"))
		{
			if (d["links"].IsArray())
			{
				GenericArray linkArray = d["links"].GetArray();

				for (uint32 l = 0; l < linkArray.Size(); l++)
				{
					GenericObject linkObject = linkArray[l].GetObject();
					EditorRenderGraphResourceLink link = {};

					link.LinkIndex			= linkObject["link_index"].GetInt();
					link.SrcAttributeIndex	= linkObject["src_attribute_index"].GetInt();
					link.DstAttributeIndex	= linkObject["dst_attribute_index"].GetInt();

					loadedResourceStateLinks[link.LinkIndex] = link;

					if (link.LinkIndex > largestLinkID) largestLinkID = link.LinkIndex;
				}
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: \"links\" member wrong type!");
				return false;
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: \"links\" member could not be found!");
			return false;
		}

		fclose(pFile);

		//Reset to clear state
		{
			m_ResourceStateGroups.clear();
			m_FinalOutput		= {};

			m_Resources.clear();
			m_RenderStageNameByInputAttributeIndex.clear();
			m_RenderStagesByName.clear();
			m_ResourceStatesByHalfAttributeIndex.clear();
			m_ResourceStateLinksByLinkIndex.clear();

			m_CurrentlyAddingRenderStage	= EPipelineStateType::NONE;
			m_CurrentlyAddingResource		= ERenderGraphResourceType::NONE;

			m_StartedLinkInfo				= {};
		}

		//Set Loaded State
		{
			s_NextNodeID		= largestNodeID + 1;
			s_NextAttributeID	= largestAttributeID + 1;
			s_NextLinkID		= largestLinkID + 1;

			m_ResourceStateGroups	= loadedResourceStateGroups;
			m_FinalOutput			= loadedFinalOutput;

			m_Resources								= loadedResources;
			m_RenderStageNameByInputAttributeIndex	= loadedRenderStageNameByInputAttributeIndex;
			m_RenderStagesByName					= loadedRenderStagesByName;
			m_ResourceStatesByHalfAttributeIndex	= loadedResourceStatesByHalfAttributeIndex;
			m_ResourceStateLinksByLinkIndex			= loadedResourceStateLinks;
		}

		//Parse the Loaded State
		ParseStructure(generateImGuiStage);

		//Set Node Positions
		SetInitialNodePositions();

		return true;
	}

	void RenderGraphEditor::SetInitialNodePositions()
	{
		if (m_GUIInitialized)
		{
			float nodeXSpace = 450.0f;

			imnodes::SetNodeGridSpacePos(m_ResourceStateGroups[TEMPORAL_RESOURCE_STATE_GROUP_INDEX].OutputNodeIndex, ImVec2(0.0f, 0.0f));
			imnodes::SetNodeGridSpacePos(m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX].OutputNodeIndex, ImVec2(0.0f, 450.0f));

			ImVec2 currentPos = ImVec2(nodeXSpace, 0.0f);

			for (auto pipelineStageIt = m_ParsedRenderGraphStructure.PipelineStageDescriptions.begin(); pipelineStageIt != m_ParsedRenderGraphStructure.PipelineStageDescriptions.end(); pipelineStageIt++)
			{
				const PipelineStageDesc* pPipelineStageDesc = &(*pipelineStageIt);

				if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER)
				{
					const RenderStageDesc* pRenderStageDesc = &m_ParsedRenderGraphStructure.RenderStageDescriptions[pPipelineStageDesc->StageIndex];

					auto renderStageIt = m_RenderStagesByName.find(pRenderStageDesc->Name);

					if (renderStageIt != m_RenderStagesByName.end())
					{
						imnodes::SetNodeGridSpacePos(renderStageIt->second.NodeIndex, currentPos);
						currentPos.x += nodeXSpace;
					}
				}
			}

			imnodes::SetNodeGridSpacePos(m_FinalOutput.NodeIndex, ImVec2(currentPos.x, 450.0f));
			imnodes::SetNodeGridSpacePos(m_ResourceStateGroups[TEMPORAL_RESOURCE_STATE_GROUP_INDEX].InputNodeIndex, ImVec2(currentPos.x, 0.0f));
		}
	}

	void RenderGraphEditor::ResetState()
	{
		//Reset to clear state
		{
			s_NextNodeID		= 0;
			s_NextAttributeID	= 0;
			s_NextLinkID		= 0;

			m_ResourceStateGroups.clear();
			m_FinalOutput = {};

			m_Resources.clear();
			m_RenderStageNameByInputAttributeIndex.clear();
			m_RenderStagesByName.clear();
			m_ResourceStatesByHalfAttributeIndex.clear();
			m_ResourceStateLinksByLinkIndex.clear();

			m_CurrentlyAddingRenderStage = EPipelineStateType::NONE;
			m_CurrentlyAddingResource = ERenderGraphResourceType::NONE;

			m_StartedLinkInfo = {};

			m_ParsedRenderGraphStructure = {};
		}

		InitDefaultResources();
	}

	bool RenderGraphEditor::ParseStructure(bool generateImGuiStage)
	{
		const EditorRenderGraphResourceState* pBackBufferFinalState = &m_ResourceStatesByHalfAttributeIndex[m_FinalOutput.BackBufferAttributeIndex / 2];

		if (pBackBufferFinalState->InputLinkIndex == -1)
		{
			m_ParsingError = "No link connected to Final Output";
			return false;
		}

		//Get the Render Stage connected to the Final Output Stage
		const String& lastRenderStageName = m_ResourceStatesByHalfAttributeIndex[m_ResourceStateLinksByLinkIndex[pBackBufferFinalState->InputLinkIndex].SrcAttributeIndex / 2].RenderStageName;

		//Check if the final Render Stage actually is a Render Stage and not a Resource State Group
		if (!IsRenderStage(lastRenderStageName))
		{
			m_ParsingError = "A valid render stage must be linked to " + m_FinalOutput.Name;
			return false;
		}

		//Reset Render Stage Weight
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStageDesc* pCurrentRenderStage = &renderStageIt->second;
			pCurrentRenderStage->Weight = 0;
		}

		//Weight Render Stages
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStageDesc* pCurrentRenderStage = &renderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				if (!RecursivelyWeightParentRenderStages(pCurrentRenderStage))
				{
					return false;
				}
			}
		}

		//Created sorted Map of Render Stages
		std::multimap<uint32, EditorRenderStageDesc*> orderedMappedRenderStages;
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStageDesc* pCurrentRenderStage = &renderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				orderedMappedRenderStages.insert({ pCurrentRenderStage->Weight, pCurrentRenderStage });
			}
		}

		TArray<RenderStageDesc>			orderedRenderStages;
		TArray<SynchronizationStageDesc>	orderedSynchronizationStages;
		TArray<PipelineStageDesc>			orderedPipelineStages;

		orderedRenderStages.reserve(orderedMappedRenderStages.size());
		orderedSynchronizationStages.reserve(orderedMappedRenderStages.size());
		orderedPipelineStages.reserve(2 * orderedMappedRenderStages.size());

		THashTable<String, const EditorRenderGraphResourceState*> finalStateOfResources;

		RenderStageDesc imguiRenderStage = {};

		if (generateImGuiStage)
		{
			imguiRenderStage.Name				= RENDER_GRAPH_IMGUI_STAGE_NAME;
			imguiRenderStage.Type				= EPipelineStateType::GRAPHICS;
			imguiRenderStage.CustomRenderer		= true;
			imguiRenderStage.Enabled			= true;
		}

		//Loop Through each Render Stage in Order and create synchronization stages
		for (auto orderedRenderStageIt = orderedMappedRenderStages.rbegin(); orderedRenderStageIt != orderedMappedRenderStages.rend(); orderedRenderStageIt++)
		{
			const EditorRenderStageDesc* pCurrentRenderStage = orderedRenderStageIt->second;

			if (pCurrentRenderStage->Enabled)
			{
				SynchronizationStageDesc synchronizationStage = {};

				//Loop through each Resource State in the Render Stage
				for (const EditorResourceStateIdent& resourceStateIdent : pCurrentRenderStage->ResourceStateIdents)
				{
					const EditorRenderGraphResourceState* pCurrentResourceState		= &m_ResourceStatesByHalfAttributeIndex[resourceStateIdent.AttributeIndex / 2];
			
					if (FindAndCreateSynchronization(generateImGuiStage, orderedRenderStageIt, orderedMappedRenderStages, pCurrentResourceState, &synchronizationStage))
					{
						finalStateOfResources[pCurrentResourceState->ResourceName] = pCurrentResourceState;
					}
				}

				if (pCurrentRenderStage->Type == EPipelineStateType::GRAPHICS) //Check if this Render Stage is a Graphics Render Stage, if it is we need to check Draw Resources as well
				{
					if (pCurrentRenderStage->Graphics.DrawType == ERenderStageDrawType::SCENE_INDIRECT)
					{
						const EditorRenderGraphResourceState* pIndexBufferResourceState			= &m_ResourceStatesByHalfAttributeIndex[pCurrentRenderStage->Graphics.IndexBufferAttributeIndex / 2];
						const EditorRenderGraphResourceState* pIndirectArgsBufferResourceState	= &m_ResourceStatesByHalfAttributeIndex[pCurrentRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];

						if (pIndexBufferResourceState->ResourceName.size() > 0)
						{
							auto resourceStateIdentIt = pCurrentRenderStage->FindResourceStateIdent(pIndexBufferResourceState->ResourceName);

							if (resourceStateIdentIt != pCurrentRenderStage->ResourceStateIdents.end())
							{
								const EditorRenderGraphResourceState* pIndexBufferDescriptorResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIdentIt->AttributeIndex / 2];

								if (pIndexBufferDescriptorResourceState->OutputLinkIndices.size() > 0)
								{
									m_ParsingError = "Draw resource \"" + pIndexBufferResourceState->ResourceName + "\" is also bound to descriptor set in write mode";
									return false;
								}
							}
							else
							{
								FindAndCreateSynchronization(generateImGuiStage, orderedRenderStageIt, orderedMappedRenderStages, pIndexBufferResourceState, &synchronizationStage);
							}
						}

						if (pIndirectArgsBufferResourceState->ResourceName.size() > 0)
						{
							auto resourceStateIdentIt = pCurrentRenderStage->FindResourceStateIdent(pIndirectArgsBufferResourceState->ResourceName);

							if (resourceStateIdentIt != pCurrentRenderStage->ResourceStateIdents.end())
							{
								const EditorRenderGraphResourceState* pIndirectArgsBufferDescriptorResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIdentIt->AttributeIndex / 2];

								if (pIndirectArgsBufferDescriptorResourceState->OutputLinkIndices.size() > 0)
								{
									m_ParsingError = "Draw resource \"" + pIndexBufferResourceState->ResourceName + "\" is also bound to descriptor set in write mode";
									return false;
								}
							}
							else
							{
								FindAndCreateSynchronization(generateImGuiStage, orderedRenderStageIt, orderedMappedRenderStages, pIndirectArgsBufferResourceState, &synchronizationStage);
							}
						}
					}
				}

				RenderStageDesc parsedRenderStage = {};
				CreateParsedRenderStage(&parsedRenderStage, pCurrentRenderStage);

				orderedRenderStages.push_back(parsedRenderStage);
				orderedPipelineStages.push_back({ ERenderGraphPipelineStageType::RENDER, uint32(orderedRenderStages.size()) - 1 });

				if (synchronizationStage.Synchronizations.size() > 0)
				{
					orderedSynchronizationStages.push_back(synchronizationStage);
					orderedPipelineStages.push_back({ ERenderGraphPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.size()) - 1 });
				}
			}
		}

		if (generateImGuiStage)
		{
			SynchronizationStageDesc imguiSynchronizationStage = {};

			for (auto resourceStateIt = finalStateOfResources.begin(); resourceStateIt != finalStateOfResources.end(); resourceStateIt++)
			{
				const EditorRenderGraphResourceState* pFinalResourceState = resourceStateIt->second;

				auto finalResourceIt = FindResource(pFinalResourceState->ResourceName);

				if (finalResourceIt != m_Resources.end())
				{
					if (CapturedByImGui(&(*finalResourceIt)))
					{
						//Todo: What if SubResourceCount > 1

						RenderGraphResourceState resourceState = {};
						resourceState.ResourceName = pFinalResourceState->ResourceName;

						if (pFinalResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
						{
							//This is just a dummy as it will be removed in a later stage
							RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
							resourceSynchronization.PrevRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
							resourceSynchronization.NextRenderStage = "PRESENT (Not a Render Stage)";
							resourceSynchronization.PrevBindingType = ERenderGraphResourceBindingType::ATTACHMENT;
							resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::PRESENT;
							resourceSynchronization.PrevQueue		= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
							resourceSynchronization.NextQueue		= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
							resourceSynchronization.ResourceName	= pFinalResourceState->ResourceName;

							imguiSynchronizationStage.Synchronizations.push_back(resourceSynchronization);

							resourceState.BindingType = ERenderGraphResourceBindingType::ATTACHMENT;
						}
						else
						{
							//If this resource is not the Back Buffer, we need to check if the following frame needs to have the resource transitioned to some initial state
							for (auto orderedRenderStageIt = orderedMappedRenderStages.rbegin(); orderedRenderStageIt != orderedMappedRenderStages.rend(); orderedRenderStageIt++)
							{
								const EditorRenderStageDesc* pPotentialNextRenderStage = orderedRenderStageIt->second;
								bool done = false;

								if (pPotentialNextRenderStage->Enabled && !done)
								{
									auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(pFinalResourceState->ResourceName);

									if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
									{
										const EditorRenderGraphResourceState* pNextResourceState = &m_ResourceStatesByHalfAttributeIndex[potentialNextResourceStateIdentIt->AttributeIndex / 2];

										RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
										resourceSynchronization.PrevRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
										resourceSynchronization.NextRenderStage = pPotentialNextRenderStage->Name;
										resourceSynchronization.PrevBindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;
										resourceSynchronization.NextBindingType = pNextResourceState->BindingType;
										resourceSynchronization.PrevQueue		= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
										resourceSynchronization.NextQueue		= ConvertPipelineStateTypeToQueue(pPotentialNextRenderStage->Type);
										resourceSynchronization.ResourceName	= pFinalResourceState->ResourceName;

										imguiSynchronizationStage.Synchronizations.push_back(resourceSynchronization);

										done = true;
										break;
									}
								}
							}

							resourceState.BindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;
						}

						imguiRenderStage.ResourceStates.push_back(resourceState);
					}
				}
				else
				{
					LOG_ERROR("[RenderGraphEditor]: Final Resource State with name \"%s\" could not be found among resources", pFinalResourceState->ResourceName);
					return false;
				}
			}

			orderedRenderStages.push_back(imguiRenderStage);
			orderedPipelineStages.push_back({ ERenderGraphPipelineStageType::RENDER, uint32(orderedRenderStages.size()) - 1 });

			orderedSynchronizationStages.push_back(imguiSynchronizationStage);
			orderedPipelineStages.push_back({ ERenderGraphPipelineStageType::SYNCHRONIZATION, uint32(orderedSynchronizationStages.size()) - 1 });
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
					if (synchronizationIt->PrevQueue == synchronizationIt->NextQueue && synchronizationIt->PrevBindingType == synchronizationIt->NextBindingType)
					{
						synchronizationIt = pSynchronizationStage->Synchronizations.erase(synchronizationIt);
						continue;
					}

					synchronizationIt++;
				}

				if (pSynchronizationStage->Synchronizations.empty())
				{
					//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
					for (auto updatePipelineStageIt = pipelineStageIt + 1; updatePipelineStageIt != orderedPipelineStages.end(); updatePipelineStageIt++)
					{
						if (updatePipelineStageIt->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
						{
							updatePipelineStageIt->StageIndex--;
						}
					}

					orderedSynchronizationStages.erase(orderedSynchronizationStages.begin() + pPipelineStageDesc->StageIndex);
					pipelineStageIt = orderedPipelineStages.erase(pipelineStageIt);

					continue;
				}
			}
			
			pipelineStageIt++;
		}

		//Do a pass to convert Barriers synchronizations to Render Pass transitions, where applicable
		for (uint32 p = 0; p < orderedPipelineStages.size(); p++)
		{
			const PipelineStageDesc* pPipelineStageDesc = &orderedPipelineStages[p];

			if (pPipelineStageDesc->Type == ERenderGraphPipelineStageType::RENDER)
			{
				RenderStageDesc* pRenderStageDesc = &orderedRenderStages[pPipelineStageDesc->StageIndex];

				//Check if this Render Stage is a Graphics Stage, if it is, we can look for Render Pass Attachments
				if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
				{
					for (auto resourceStateIt = pRenderStageDesc->ResourceStates.begin(); resourceStateIt != pRenderStageDesc->ResourceStates.end(); resourceStateIt++)
					{
						RenderGraphResourceState* pResourceState = &(*resourceStateIt);

						bool isBackBuffer = pResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

						//Check if this Resource State has a binding type of ATTACHMENT, if it does, we need to modify the surrounding barriers and the internal Previous- and Next States of the Resource State
						if (pResourceState->BindingType == ERenderGraphResourceBindingType::ATTACHMENT)
						{
							bool													prevSameFrame									= true;
							RenderGraphResourceState*								pPreviousResourceStateDesc						= nullptr;
							int32													previousSynchronizationPipelineStageDescIndex	= -1;
							TArray<RenderGraphResourceSynchronizationDesc>::iterator	previousSynchronizationDescIt;

							RenderGraphResourceState*								pNextResourceStateDesc							= nullptr;
							int32													nextSynchronizationPipelineStageDescIndex		= -1;
							TArray<RenderGraphResourceSynchronizationDesc>::iterator nextSynchronizationDescIt;

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
										pp = orderedPipelineStages.size() - 1;
										prevSameFrame = false;

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
											previousSynchronizationPipelineStageDescIndex	= pp;
											previousSynchronizationDescIt					= prevSynchronizationIt;
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
								if (np >= orderedPipelineStages.size())
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
											nextSynchronizationPipelineStageDescIndex	= np;
											nextSynchronizationDescIt					= nextSynchronizationIt;
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
								pResourceState->AttachmentSynchronizations.PrevSameFrame	= prevSameFrame;
							}
							else
							{
								pResourceState->AttachmentSynchronizations.PrevBindingType = ERenderGraphResourceBindingType::NONE;
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
									LOG_ERROR("[RenderGraphEditor]: Resource \"%s\" is used as an attachment in Render Stage \"%s\" but is not used in later stages", pResourceState->ResourceName.c_str(), pRenderStageDesc->Name.c_str());
									pResourceState->AttachmentSynchronizations.NextBindingType = ERenderGraphResourceBindingType::NONE;
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
									pPreviousSynchronizationStage->Synchronizations.erase(previousSynchronizationDescIt);

									if (pPreviousSynchronizationStage->Synchronizations.empty())
									{
										//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
										for (int32 up = previousSynchronizationPipelineStageDescIndex + 1; up < orderedPipelineStages.size(); up++)
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

										orderedSynchronizationStages.erase(orderedSynchronizationStages.begin() + orderedPipelineStages[previousSynchronizationPipelineStageDescIndex].StageIndex);
										orderedPipelineStages.erase(orderedPipelineStages.begin() + previousSynchronizationPipelineStageDescIndex);
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
									pNextSynchronizationStage->Synchronizations.erase(nextSynchronizationDescIt);

									if (pNextSynchronizationStage->Synchronizations.empty())
									{
										//If we remove a synchronization stage, the following Pipeline Stages that are Synchronization Stages will need to have their index updateds
										for (int32 up = nextSynchronizationPipelineStageDescIndex + 1; up < orderedPipelineStages.size(); up++)
										{
											PipelineStageDesc* pUpdatePipelineStageDesc = &orderedPipelineStages[up];

											if (pUpdatePipelineStageDesc->Type == ERenderGraphPipelineStageType::SYNCHRONIZATION)
											{
												pUpdatePipelineStageDesc->StageIndex--;
											}
										}

										orderedSynchronizationStages.erase(orderedSynchronizationStages.begin() + orderedPipelineStages[nextSynchronizationPipelineStageDescIndex].StageIndex);
										orderedPipelineStages.erase(orderedPipelineStages.begin() + nextSynchronizationPipelineStageDescIndex);
									}
								}
							}
						}
					}
				}
			}
		}

		//Do another pass over all Render Stages to set Resource Flags
		for (uint32 r = 0; r < orderedRenderStages.size(); r++)
		{
			RenderStageDesc* pRenderStageDesc = &orderedRenderStages[r];

			for (uint32 rs = 0; rs < pRenderStageDesc->ResourceStates.size(); rs++)
			{
				RenderGraphResourceState* pResourceState = &pRenderStageDesc->ResourceStates[rs];

				auto resourceIt = FindResource(pResourceState->ResourceName);

				if (resourceIt != m_Resources.end())
				{
					switch (resourceIt->Type)
					{
						case ERenderGraphResourceType::TEXTURE:
						{
							switch (pResourceState->BindingType)
							{
							case ERenderGraphResourceBindingType::COMBINED_SAMPLER:
								resourceIt->TextureParams.TextureFlags		|= FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE; 
								resourceIt->TextureParams.TextureViewFlags	|= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE; 
								break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		
								resourceIt->TextureParams.TextureFlags		|= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS; 
								resourceIt->TextureParams.TextureViewFlags	|= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS; 
								break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		
								resourceIt->TextureParams.TextureFlags		|= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS; 
								resourceIt->TextureParams.TextureViewFlags	|= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS; 
								break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	
								resourceIt->TextureParams.TextureFlags		|= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS; 
								resourceIt->TextureParams.TextureViewFlags	|= FTextureViewFlags::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS; 
								break;
							case ERenderGraphResourceBindingType::ATTACHMENT:
							{
								bool isDepthStencilAttachment = resourceIt->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT;
								resourceIt->TextureParams.TextureFlags		|= (isDepthStencilAttachment ? FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL			: FTextureFlags::TEXTURE_FLAG_RENDER_TARGET);
								resourceIt->TextureParams.TextureViewFlags	|= (isDepthStencilAttachment ? FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL	: FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET);
								break;
							}
							}
							break;
						}
						case ERenderGraphResourceType::BUFFER:
						{
							switch (pResourceState->BindingType)
							{		
							case ERenderGraphResourceBindingType::CONSTANT_BUFFER:				resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER; break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ:		resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE:		resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
							case ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE:	resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER; break;
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

			//Draw Resources
			if (pRenderStageDesc->Type == EPipelineStateType::GRAPHICS)
			{
				if (pRenderStageDesc->Graphics.DrawType == ERenderStageDrawType::SCENE_INDIRECT)
				{
					if (!pRenderStageDesc->Graphics.IndexBufferName.empty())
					{
						auto resourceIt = FindResource(pRenderStageDesc->Graphics.IndexBufferName);

						if (resourceIt != m_Resources.end())
						{
							resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
						}
					}

					if (!pRenderStageDesc->Graphics.IndirectArgsBufferName.empty())
					{
						auto resourceIt = FindResource(pRenderStageDesc->Graphics.IndirectArgsBufferName);

						if (resourceIt != m_Resources.end())
						{
							resourceIt->BufferParams.BufferFlags |= FBufferFlags::BUFFER_FLAG_INDIRECT_BUFFER;
						}
					}
				}
			}
		}

		m_ParsedRenderGraphStructure.ResourceDescriptions				= m_Resources;
		m_ParsedRenderGraphStructure.RenderStageDescriptions			= orderedRenderStages;
		m_ParsedRenderGraphStructure.SynchronizationStageDescriptions	= orderedSynchronizationStages;
		m_ParsedRenderGraphStructure.PipelineStageDescriptions			= orderedPipelineStages;

		return true;
	}

	bool RenderGraphEditor::RecursivelyWeightParentRenderStages(EditorRenderStageDesc* pChildRenderStage)
	{
		std::set<String> parentRenderStageNames;

		//Iterate through all resource states in the current Render Stages
		for (const EditorResourceStateIdent& resourceStateIdent : pChildRenderStage->ResourceStateIdents)
		{
			const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIdent.AttributeIndex / 2];

			//Check if resource state has input link
			if (pResourceState->InputLinkIndex != -1)
			{
				const String& parentRenderStageName = m_ResourceStatesByHalfAttributeIndex[m_ResourceStateLinksByLinkIndex[pResourceState->InputLinkIndex].SrcAttributeIndex / 2].RenderStageName;

				//Make sure parent is a Render Stage and check if it has already been visited
				if (IsRenderStage(parentRenderStageName) && parentRenderStageNames.count(parentRenderStageName) == 0)
				{
					EditorRenderStageDesc* pParentRenderStage = &m_RenderStagesByName[parentRenderStageName];
					RecursivelyWeightParentRenderStages(pParentRenderStage);

					parentRenderStageNames.insert(parentRenderStageName);
					pParentRenderStage->Weight++;
				}
			}
		}

		return true;
	}

	bool RenderGraphEditor::IsRenderStage(const String& name)
	{
		return m_RenderStagesByName.count(name) > 0;
	}

	bool RenderGraphEditor::CapturedByImGui(const RenderGraphResourceDesc* pResource)
	{
		return pResource->Type == ERenderGraphResourceType::TEXTURE && pResource->SubResourceCount == 1;
	}

	bool RenderGraphEditor::FindAndCreateSynchronization(
		bool generateImGuiStage,
		const std::multimap<uint32, EditorRenderStageDesc*>::reverse_iterator& currentOrderedRenderStageIt,
		const std::multimap<uint32, EditorRenderStageDesc*>& orderedMappedRenderStages,
		const EditorRenderGraphResourceState* pCurrentResourceState,
		SynchronizationStageDesc* pSynchronizationStage)
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
						
			//Check if this Render Stage is enabled
			if (pPotentialNextRenderStage->Enabled)
			{
				//See if this Render Stage uses Resource we are looking for
				auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(pCurrentResourceState->ResourceName);

				if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
				{
					pNextResourceState = &m_ResourceStatesByHalfAttributeIndex[potentialNextResourceStateIdentIt->AttributeIndex / 2];
					pNextRenderStage = pPotentialNextRenderStage;
					break;
				}
				else if (pPotentialNextRenderStage->Type == EPipelineStateType::GRAPHICS) //Check if this Render Stage is a Graphics Render Stage, if it is we need to check Draw Resources as well
				{
					if (pPotentialNextRenderStage->Graphics.DrawType == ERenderStageDrawType::SCENE_INDIRECT)
					{
						const EditorRenderGraphResourceState* pIndexBufferResourceState				= &m_ResourceStatesByHalfAttributeIndex[pPotentialNextRenderStage->Graphics.IndexBufferAttributeIndex / 2];
						const EditorRenderGraphResourceState* pIndirectArgsBufferResourceState		= &m_ResourceStatesByHalfAttributeIndex[pPotentialNextRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];

						if (pCurrentResourceState->ResourceName == pIndexBufferResourceState->ResourceName || pCurrentResourceState->ResourceName == pIndirectArgsBufferResourceState->ResourceName)
						{
							pNextRenderStage	= pPotentialNextRenderStage;
							break;
						}
					}
				}
			}
		}

		//If there is a Next State for the Resource, pNextResourceState will not be nullptr 
		RenderGraphResourceSynchronizationDesc resourceSynchronization = {};
		resourceSynchronization.PrevRenderStage = pCurrentRenderStage->Name;
		resourceSynchronization.ResourceName	= pCurrentResourceState->ResourceName;
		resourceSynchronization.PrevQueue		= ConvertPipelineStateTypeToQueue(pCurrentRenderStage->Type);
		resourceSynchronization.PrevBindingType	= pCurrentResourceState->BindingType;

		bool isBackBuffer = pCurrentResourceState->ResourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;

		auto resourceIt = FindResource(pCurrentResourceState->ResourceName);

		if (resourceIt != m_Resources.end())
		{
			if (pNextResourceState != nullptr)
			{
				//If the ResourceState is Readonly and the current and next Binding Types are the same we dont want a synchronization, no matter what queue type
				if (!IsReadOnly(pCurrentResourceState->BindingType) || pCurrentResourceState->BindingType != pNextResourceState->BindingType)
				{
					//Check if pNextResourceState belongs to a Render Stage, otherwise we need to check if it belongs to Final Output
					if (pNextRenderStage != nullptr)
					{
						resourceSynchronization.NextRenderStage = pNextRenderStage->Name;
						resourceSynchronization.NextQueue = ConvertPipelineStateTypeToQueue(pNextRenderStage->Type);
						resourceSynchronization.NextBindingType = pNextResourceState->BindingType;

						pSynchronizationStage->Synchronizations.push_back(resourceSynchronization);
					}
				}
			}
			else if (generateImGuiStage && CapturedByImGui(&(*resourceIt)))
			{
				if (isBackBuffer)
				{
					resourceSynchronization.NextRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
					resourceSynchronization.NextQueue = ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
					resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::ATTACHMENT;

					pSynchronizationStage->Synchronizations.push_back(resourceSynchronization);
				}
				else if (!IsReadOnly(pCurrentResourceState->BindingType) || pCurrentResourceState->BindingType != ERenderGraphResourceBindingType::COMBINED_SAMPLER)
				{
					//Todo: What if Subresource Count > 1
					//Capture resource synchronizations here, even for Back Buffer, PRESENT Synchronization is seperately solved later
					resourceSynchronization.NextRenderStage = RENDER_GRAPH_IMGUI_STAGE_NAME;
					resourceSynchronization.NextQueue = ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
					resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::COMBINED_SAMPLER;

					pSynchronizationStage->Synchronizations.push_back(resourceSynchronization);
				}
			}
			else if (isBackBuffer)
			{
				resourceSynchronization.NextQueue = ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
				resourceSynchronization.NextBindingType = ERenderGraphResourceBindingType::PRESENT;

				pSynchronizationStage->Synchronizations.push_back(resourceSynchronization);
			}
			else
			{
				//If there is no following Render Stage that uses the Resource and it is not captured by ImGui and it is not the back buffer, we need to check the previous ones to discover synchronizations for the next frame
				for (auto previousOrderedRenderStageIt = orderedMappedRenderStages.rbegin(); previousOrderedRenderStageIt != currentOrderedRenderStageIt; previousOrderedRenderStageIt++)
				{
					const EditorRenderStageDesc* pPotentialNextRenderStage = previousOrderedRenderStageIt->second;

					//Check if this Render Stage is enabled
					if (pPotentialNextRenderStage->Enabled)
					{
						//See if this Render Stage uses Resource we are looking for
						auto potentialNextResourceStateIdentIt = pPotentialNextRenderStage->FindResourceStateIdent(pCurrentResourceState->ResourceName);

						if (potentialNextResourceStateIdentIt != pPotentialNextRenderStage->ResourceStateIdents.end())
						{
							pNextResourceState = &m_ResourceStatesByHalfAttributeIndex[potentialNextResourceStateIdentIt->AttributeIndex / 2];
							pNextRenderStage = pPotentialNextRenderStage;
							break;
						}
						else if (pPotentialNextRenderStage->Type == EPipelineStateType::GRAPHICS) //Check if this Render Stage is a Graphics Render Stage, if it is we need to check Draw Resources as well
						{
							if (pPotentialNextRenderStage->Graphics.DrawType == ERenderStageDrawType::SCENE_INDIRECT)
							{
								const EditorRenderGraphResourceState* pIndexBufferResourceState = &m_ResourceStatesByHalfAttributeIndex[pPotentialNextRenderStage->Graphics.IndexBufferAttributeIndex / 2];
								const EditorRenderGraphResourceState* pIndirectArgsBufferResourceState = &m_ResourceStatesByHalfAttributeIndex[pPotentialNextRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];

								if (pCurrentResourceState->ResourceName == pIndexBufferResourceState->ResourceName || pCurrentResourceState->ResourceName == pIndirectArgsBufferResourceState->ResourceName)
								{
									pNextRenderStage = pPotentialNextRenderStage;
									break;
								}
							}
						}
					}
				}

				//It is safe to add this synchronization here, since we know that the resource will not be captured by ImGui
				if (pNextResourceState != nullptr)
				{
					//If the ResourceState is Readonly and the current and next Binding Types are the same we dont want a synchronization, no matter what queue type
					if (!IsReadOnly(pCurrentResourceState->BindingType) || pCurrentResourceState->BindingType != pNextResourceState->BindingType)
					{
						//Check if pNextResourceState belongs to a Render Stage, otherwise we need to check if it belongs to Final Output
						if (pNextRenderStage != nullptr)
						{
							resourceSynchronization.NextRenderStage = pNextRenderStage->Name;
							resourceSynchronization.NextQueue = ConvertPipelineStateTypeToQueue(pNextRenderStage->Type);
							resourceSynchronization.NextBindingType = pNextResourceState->BindingType;

							pSynchronizationStage->Synchronizations.push_back(resourceSynchronization);
						}
					}
				}
			}
		}
		else
		{ 
			LOG_ERROR("[RenderGraphEditor]: Resource State with name \"%s\" could not be found among resources when creating Synchronization", pCurrentResourceState->ResourceName);
			return false;
		}

		return true;
	}

	void RenderGraphEditor::CreateParsedRenderStage(RenderStageDesc* pDstRenderStage, const EditorRenderStageDesc* pSrcRenderStage)
	{
		pDstRenderStage->Name					= pSrcRenderStage->Name;
		pDstRenderStage->Type					= pSrcRenderStage->Type;
		pDstRenderStage->CustomRenderer			= pSrcRenderStage->CustomRenderer;
		pDstRenderStage->Enabled				= pSrcRenderStage->Enabled;
		pDstRenderStage->Parameters				= pSrcRenderStage->Parameters;

		pDstRenderStage->Weight					= pSrcRenderStage->Weight;
		pDstRenderStage->ResourceStates.reserve(pSrcRenderStage->ResourceStateIdents.size());

		for (const EditorResourceStateIdent& resourceStateIdent : pSrcRenderStage->ResourceStateIdents)
		{
			const EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[resourceStateIdent.AttributeIndex / 2];
			
			RenderGraphResourceState resourceState = {};
			resourceState.ResourceName	= pResourceState->ResourceName;
			resourceState.BindingType	= pResourceState->BindingType;

			pDstRenderStage->ResourceStates.push_back(resourceState);
		}

		if (pDstRenderStage->Type == EPipelineStateType::GRAPHICS)
		{
			pDstRenderStage->Graphics.Shaders					= pSrcRenderStage->Graphics.Shaders;
			pDstRenderStage->Graphics.DrawType					= pSrcRenderStage->Graphics.DrawType;
			pDstRenderStage->Graphics.IndexBufferName			= m_ResourceStatesByHalfAttributeIndex[pSrcRenderStage->Graphics.IndexBufferAttributeIndex / 2].ResourceName;
			pDstRenderStage->Graphics.IndirectArgsBufferName	= m_ResourceStatesByHalfAttributeIndex[pSrcRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2].ResourceName;
			pDstRenderStage->Graphics.DepthTestEnabled			= pSrcRenderStage->Graphics.DepthTestEnabled;
		}
		else if (pDstRenderStage->Type == EPipelineStateType::COMPUTE)
		{
			pDstRenderStage->Compute.ShaderName					= pSrcRenderStage->Compute.ShaderName;
		}
		else if (pDstRenderStage->Type == EPipelineStateType::RAY_TRACING)
		{
			pDstRenderStage->RayTracing.Shaders					= pSrcRenderStage->RayTracing.Shaders;
		}
	}

}