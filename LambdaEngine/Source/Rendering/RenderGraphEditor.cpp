#include "Rendering/RenderGraphEditor.h"

#include "Application/API/CommonApplication.h"

#include "Application/API/Events/EventQueue.h"

#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Log/Log.h"

#include "Utilities/IOUtilities.h"
#include "Time/API/Clock.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include "Rendering/Renderer.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraphParser.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderGraphEditorHelpers.h"

namespace LambdaEngine
{
	int32 RenderGraphEditor::s_NextNodeID		= 0;
	int32 RenderGraphEditor::s_NextAttributeID	= 0;
	int32 RenderGraphEditor::s_NextLinkID		= 0;

	constexpr const uint32 NODE_TITLE_COLOR[4] =
	{
		IM_COL32(69, 159, 59, 255),		// Forest Green
		IM_COL32(230, 149, 55, 255),	// Orange
		IM_COL32(171, 21, 209, 255),	// Purple
		IM_COL32(41, 74, 122, 255),		// ImGui Blue
	};

	constexpr const uint32 HOVERED_COLOR[4] =
	{
		IM_COL32(46, 104, 40, 255),		// Dark Forest Green
		IM_COL32(186, 122, 48, 255),	// Dark Orange
		IM_COL32(106, 13, 129, 255),	// Dark Purple
		IM_COL32(33, 60, 100, 255),		// Dark ImGui Blue
	};

	constexpr const uint32 SELECTED_COLOR[4] =
	{
		IM_COL32(124, 222, 40, 113),	// Light Forest Green
		IM_COL32(245, 175, 96, 255),	// Light Orange
		IM_COL32(207, 93, 236, 255),	// Light Purple
		IM_COL32(65, 122, 206, 255),	// Light ImGui Blue
	};

	constexpr const uint32 EXTERNAL_RESOURCE_STATE_GROUP_INDEX = 0;
	constexpr const uint32 TEMPORAL_RESOURCE_STATE_GROUP_INDEX = 1;
	constexpr const uint32 NUM_RESOURCE_STATE_GROUPS = 2;

	constexpr const int32 MAX_RESOURCE_NAME_LENGTH				= 256;

	RenderGraphEditor::RenderGraphEditor()
	{
		EventQueue::RegisterEventHandler<MouseButtonReleasedEvent>(this, &RenderGraphEditor::OnButtonReleased);
		EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &RenderGraphEditor::OnKeyPressed);
		EventQueue::RegisterEventHandler<KeyReleasedEvent>(this, &RenderGraphEditor::OnKeyReleased);

		InitDefaultResources();
	}

	RenderGraphEditor::~RenderGraphEditor()
	{
		EventQueue::UnregisterEventHandler<MouseButtonReleasedEvent>(this, &RenderGraphEditor::OnButtonReleased);
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &RenderGraphEditor::OnKeyPressed);
		EventQueue::UnregisterEventHandler<KeyReleasedEvent>(this, &RenderGraphEditor::OnKeyReleased);
	}

	void RenderGraphEditor::InitGUI()
	{
		imnodes::StyleColorsDark();

		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, HOVERED_COLOR[0]);
		imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, SELECTED_COLOR[0]);

		imnodes::PushColorStyle(imnodes::ColorStyle_LinkHovered, HOVERED_COLOR[0]);
		imnodes::PushColorStyle(imnodes::ColorStyle_LinkSelected, SELECTED_COLOR[0]);

		m_GUIInitialized = true;
		ImGui::GetIO().FontAllowUserScaling = true;

		SetInitialNodePositions();
	}

	void RenderGraphEditor::Update()
	{
		if (m_ApplyRenderGraph)
		{
			Renderer::SetRenderGraph("Dynamically Applied RenderGraph", &m_ParsedRenderGraphStructure);
			m_ApplyRenderGraph = false;
		}
	}

	void RenderGraphEditor::RenderGUI()
	{
		if (ImGui::Begin("Render Graph Editor"))
		{
			ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
			ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

			float contentRegionWidth = contentRegionMax.x - contentRegionMin.x;
			float contentRegionHeight = contentRegionMax.y - contentRegionMin.y;

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
			ImVec2 resourceViewSize(maxResourcesViewTextWidth + editButtonWidth + removeButtonWidth, contentRegionHeight);

			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;

			static ImVec2 childSize0 = resourceViewSize;
			static ImVec2 childSize1 = ImVec2(contentRegionWidth - resourceViewSize.x, 0.0f);

			ImRect bb;
			bb.Min = ImVec2(window->DC.CursorPos.x + childSize0.x, window->DC.CursorPos.y);
			bb.Max = ImVec2(+window->DC.CursorPos.x + childSize0.x + 10.f, window->DC.CursorPos.y + contentRegionHeight);
			ImGui::SplitterBehavior(bb, ImGui::GetID("Render Graph Editor"), ImGuiAxis_X, &childSize0.x, &childSize1.x, 200.f, 500.f);

			if (ImGui::BeginChild("##Graph Resources View", childSize0))
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

			if (ImGui::BeginChild("##Graph Views", childSize1))
			{
				if (ImGui::BeginTabBar("##Render Graph Editor Tabs"))
				{
					if (ImGui::BeginTabItem("Graph Editor"))
					{
						if (ImGui::BeginChild("##Graph Editor View", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_MenuBar))
						{
							RenderGraphView();
							RenderNewRenderGraphConfirmationView();
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
			if (!RenderGraphParser::ParseRenderGraph(
				&m_ParsedRenderGraphStructure,
				m_Resources,
				m_RenderStagesByName,
				m_ResourceStatesByHalfAttributeIndex,
				m_ResourceStateLinksByLinkIndex,
				m_FinalOutput,
				true))
			{
				m_ParsedGraphValid = false;
				LOG_ERROR("[RenderGraphEditor]: Failed to parse RenderGraph");
			}
			else
			{
				m_ParsedGraphValid = true;
			}

			m_ParsedGraphDirty = false;
			m_ParsedGraphRenderDirty = true;
		}
	}

	bool RenderGraphEditor::OnButtonReleased(const MouseButtonReleasedEvent& event)
	{
		//imnodes seems to be bugged when releasing a link directly after starting creation, so we check this here
		if (event.Button == EMouseButton::MOUSE_BUTTON_LEFT)
		{
			if (m_StartedLinkInfo.LinkStarted)
			{
				m_StartedLinkInfo = {};
			}
		}

		return true;
	}

	bool RenderGraphEditor::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.Key == EKey::KEY_LEFT_SHIFT && !event.IsRepeat)
		{
			imnodes::PushAttributeFlag(imnodes::AttributeFlags_EnableLinkDetachWithDragClick);
		}

		return true;
	}

	bool RenderGraphEditor::OnKeyReleased(const KeyReleasedEvent& event)
	{
		if (event.Key == EKey::KEY_LEFT_SHIFT)
		{
			imnodes::PopAttributeFlag();
		}

		return true;
	}

	void RenderGraphEditor::InitDefaultResources()
	{
		m_FilesInShaderDirectory = EnumerateFilesInDirectory("../Assets/Shaders/", true);
		m_FilesInShaderMap = ExtractDirectory("../Assets/Shaders", "\\");

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
		//imguiRenderStage.Type					= EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS;
		//imguiRenderStage.CustomRenderer			= true;
		//imguiRenderStage.Enabled				= true;

		//m_RenderStageNameByInputAttributeIndex[imguiRenderStage.InputAttributeIndex] = imguiRenderStage.Name;
		//m_RenderStagesByName[imguiRenderStage.Name] = imguiRenderStage;

		s_NextAttributeID += 2;

		{
			RenderGraphResourceDesc resource = RenderGraphParser::CreateBackBufferResource();
			m_Resources.PushBack(resource);
			m_FinalOutput.BackBufferAttributeIndex = CreateResourceState(resource.Name, m_FinalOutput.Name, false, ERenderGraphResourceBindingType::NONE).AttributeIndex;
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= FULLSCREEN_QUAD_VERTEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= PER_FRAME_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_LIGHTS_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_MAT_PARAM_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_VERTEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_INDEX_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_PRIMARY_INSTANCE_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_SECONDARY_INSTANCE_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_INDIRECT_ARGS_BUFFER;
			resource.Type						= ERenderGraphResourceType::BUFFER;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name						= SCENE_TLAS;
			resource.Type						= ERenderGraphResourceType::ACCELERATION_STRUCTURE;
			resource.SubResourceCount			= 1;
			resource.Editable					= false;
			resource.External					= true;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_ALBEDO_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_NORMAL_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_AO_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_ROUGHNESS_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		{
			RenderGraphResourceDesc resource = {};
			resource.Name							= SCENE_METALLIC_MAPS;
			resource.Type							= ERenderGraphResourceType::TEXTURE;
			resource.SubResourceCount				= MAX_UNIQUE_MATERIALS;
			resource.Editable						= false;
			resource.External						= true;
			resource.TextureParams.TextureFormat	= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_Resources.PushBack(resource);

			externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(resource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
		}

		m_ResourceStateGroups.Resize(NUM_RESOURCE_STATE_GROUPS);
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
				if (ImGui::BeginMenu("Add Texture"))
				{
					if (ImGui::MenuItem("2D Texture", NULL, nullptr))
					{
						openAddResourcePopup = true;
						m_CurrentlyAddingResource = ERenderGraphResourceType::TEXTURE;
						m_CurrentlyAddingTextureType = ERenderGraphTextureType::TEXTURE_2D;

					}

					if (ImGui::MenuItem("Cube Texture", NULL, nullptr))
					{
						openAddResourcePopup = true;
						m_CurrentlyAddingResource = ERenderGraphResourceType::TEXTURE;
						m_CurrentlyAddingTextureType = ERenderGraphTextureType::TEXTURE_CUBE;
					}

					ImGui::EndMenu();
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
		//ImGui::Columns(2);

		static int32 selectedResourceIndex			= -1;
		static RenderGraphResourceDesc* pSelectedResource	= nullptr;
		static int32 removedResourceIndex			= -1;
		static RenderGraphResourceDesc* pRemovedResource		= nullptr;

		for (int32 i = 0; i < int32(m_Resources.GetSize()); i++)
		{
			RenderGraphResourceDesc* pResource = &m_Resources[i];

			if (ImGui::Selectable(pResource->Name.c_str(), selectedResourceIndex == i, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
			{
				selectedResourceIndex	= i;
				pSelectedResource		= pResource;
			}

			if (ImGui::IsItemHovered())
			{
				//ImGui::NextColumn();
				pSelectedResource = pResource;

				String tooltip;

				String resourceType = RenderGraphResourceTypeToString(pSelectedResource->Type);
				tooltip.append("Type" + resourceType + "\n");

				String subResourceCount;

				if (pSelectedResource->BackBufferBound)
				{
					subResourceCount = "Back Buffer Bound";
				}
				else
				{
					subResourceCount = std::to_string(pSelectedResource->SubResourceCount);
				}

				tooltip.append("Sub Resource Count: " + subResourceCount + "\n");

				if (pSelectedResource->Type == ERenderGraphResourceType::TEXTURE)
				{
					if (pSelectedResource->SubResourceCount > 1)
					{
						String temp = pSelectedResource->TextureParams.IsOfArrayType ? "True" : "False";
						tooltip.append("Is of Array Type: " + temp + "\n");
					}

					int32 textureFormatIndex = TextureFormatToFormatIndex(pSelectedResource->TextureParams.TextureFormat);

					if (textureFormatIndex >= 0)
					{
						tooltip.append("Texture Format: " + String(TEXTURE_FORMAT_NAMES[textureFormatIndex]) + "\n");
					}
					else
					{
						tooltip.append("Texture Format: INVALID\n");
					}
				}

				ImGui::SetTooltip(tooltip.c_str());
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

		/*if (pSelectedResource != nullptr)
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
		}*/

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

			m_Resources.Erase(m_Resources.begin() + removedResourceIndex);

			removedResourceIndex	= -1;
			pRemovedResource		= nullptr;
		}

		if (openAddResourcePopup)
			ImGui::OpenPopup("Add Resource ##Popup");
		if (openEditResourcePopup)
			ImGui::OpenPopup("Edit Resource ##Popup");
	}

	void RenderGraphEditor::RenderNewRenderGraphConfirmationView()
	{
		if (ImGui::BeginPopupModal("New Render Graph Confirmation ##Popup"))
		{
			ImGui::Text("Are you sure you want to create a new RenderGraph?");
			ImGui::Text("This will discard the current RenderGraph.");
			ImGui::NewLine();

			bool done = false;

			if (ImGui::Button("Yes I'm Sure!"))
			{
				ResetState();
				SetInitialNodePositions();
				done = true;
			}
			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				done = true;
			}

			if (done)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
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
					addingResource.TextureParams.TextureType = m_CurrentlyAddingTextureType;

					m_Resources.PushBack(addingResource);

					if (addingResource.External)
					{
						EditorResourceStateGroup& externalResourcesGroup = m_ResourceStateGroups[EXTERNAL_RESOURCE_STATE_GROUP_INDEX];
						externalResourcesGroup.ResourceStateIdents.PushBack(CreateResourceState(addingResource.Name, externalResourcesGroup.Name, false, ERenderGraphResourceBindingType::NONE));
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
		static TArray<RenderGraphResourceDesc>::Iterator editedResourceIt;
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
						pExternalResourcesGroup->ResourceStateIdents.PushBack(CreateResourceState(resourceNameBuffer, pExternalResourcesGroup->Name, false, ERenderGraphResourceBindingType::NONE));
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

		pResource->TextureParams.TextureType = m_CurrentlyAddingTextureType;
		
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

					if (m_CurrentlyAddingTextureType != ERenderGraphTextureType::TEXTURE_CUBE)
					{
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
					}
					else 
					{
						ImGui::Text("Width & Height: ");
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
							pResource->TextureParams.YDimVariable = pResource->TextureParams.XDimVariable;
						}
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

		RenderShaderTreeView(m_FilesInShaderMap, textWidth, textHeight, selectedResourceIndex);

		//for (auto fileIt = m_FilesInShaderDirectory.begin(); fileIt != m_FilesInShaderDirectory.end(); fileIt++)
		//{
		//	std::iterator_traits<TArray<std::string>::Iterator>::difference_type v;

		//	int32 index = std::distance(m_FilesInShaderDirectory.begin(), fileIt);
		//	const String* pFilename = &(*fileIt);

		//	//if (pFilename->find(".glsl") != String::npos)
		//	{
		//		if (ImGui::Selectable(pFilename->c_str(), selectedResourceIndex == index, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
		//		{
		//			selectedResourceIndex = index;
		//		}

		//		if (ImGui::BeginDragDropSource())
		//		{
		//			ImGui::SetDragDropPayload("SHADER", &pFilename, sizeof(const String*));
		//			ImGui::EndDragDropSource();
		//		}
		//	}
		//}
	}

	void RenderGraphEditor::RenderShaderTreeView(const LambdaDirectory& dir, float textWidth, float textHeight, int32& selectedIndex)
	{
		if (ImGui::TreeNode(dir.RelativePath.filename().string().c_str()))
		{
			for (auto entry = dir.Children.begin(); entry != dir.Children.end(); entry++)
			{
				std::iterator_traits<TArray<std::string>::Iterator>::difference_type v;

				int32 index = std::distance(dir.Children.begin(), entry);
				auto* pPath = &(entry->RelativePath);

				if (entry->isDirectory)
				{
					RenderShaderTreeView(*entry, textWidth, textHeight, selectedIndex);
				}
				else
				{
					ImGui::Bullet();
					if (ImGui::Selectable(pPath->filename().string().c_str(), selectedIndex == index, ImGuiSeparatorFlags_None, ImVec2(textWidth, textHeight)))
					{
						selectedIndex = index;
					}

					if (ImGui::BeginDragDropSource())
					{
						
						ImGui::SetDragDropPayload("SHADER", &pPath, sizeof(const std::filesystem::path*));
						ImGui::EndDragDropSource();
					}
				}
			}
			ImGui::TreePop();
		}
	}

	void RenderGraphEditor::RenderGraphView()
	{
		bool openNewRenderGraphPopup = false;
		bool openAddRenderStagePopup = false;
		bool openSaveRenderStagePopup = false;
		bool openLoadRenderStagePopup = false;

		String renderStageToDelete = "";

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("New"))
				{
					openNewRenderGraphPopup = true;
				}

				if (ImGui::MenuItem("Save", NULL, nullptr))
				{
					openSaveRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Load", NULL, nullptr))
				{
					openLoadRenderStagePopup = true;
				}

				ImGui::NewLine();

				if (ImGui::MenuItem("Apply", NULL, nullptr))
				{
					m_ParsedGraphValid = RenderGraphParser::ParseRenderGraph(
						&m_ParsedRenderGraphStructure,
						m_Resources,
						m_RenderStagesByName,
						m_ResourceStatesByHalfAttributeIndex,
						m_ResourceStateLinksByLinkIndex,
						m_FinalOutput,
						true);

					if (m_ParsedGraphValid)
					{
						m_ApplyRenderGraph = true;
					}
					else
					{
						LOG_ERROR("[RenderGraphEditor]: Failed to parse RenderGraph");
					}
				}

				if (ImGui::IsItemHovered() && !m_ParsedGraphValid)
				{
					ImGui::SetTooltip("Render Graph is Invalid");
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Add Graphics Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Compute Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE;
					openAddRenderStagePopup = true;
				}

				if (ImGui::MenuItem("Add Ray Tracing Render Stage", NULL, nullptr))
				{
					m_CurrentlyAddingRenderStage = EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING;
					openAddRenderStagePopup = true;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		//Resource State Groups
		for (uint32 resourceStateGroupIndex = 0; resourceStateGroupIndex < m_ResourceStateGroups.GetSize(); resourceStateGroupIndex++)
		{
			EditorResourceStateGroup* pResourceStateGroup = &m_ResourceStateGroups[resourceStateGroupIndex];


			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, NODE_TITLE_COLOR[3U]);
			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered, HOVERED_COLOR[3U]);
			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected, SELECTED_COLOR[3U]);
			imnodes::BeginNode(pResourceStateGroup->OutputNodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::Text((pResourceStateGroup->Name + "_OUTPUT").c_str());
			imnodes::EndNodeTitleBar();

			String resourceStateToRemove = "";

			for (const EditorResourceStateIdent& resourceStateIdent : pResourceStateGroup->ResourceStateIdents)
			{
				uint32 primaryAttributeIndex	= resourceStateIdent.AttributeIndex / 2;
				uint32 inputAttributeIndex		= resourceStateIdent.AttributeIndex;
				uint32 outputAttributeIndex		= inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, pResourceState, inputAttributeIndex);
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
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, pResourceState, inputAttributeIndex);
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
							pResourceStateGroup->ResourceStateIdents.PushBack(CreateResourceState(pResource->Name, pResourceStateGroup->Name, true, ERenderGraphResourceBindingType::NONE));
							m_ParsedGraphDirty = true;
						}
					}

					ImGui::EndDragDropTarget();
				}
			}

			imnodes::EndNode();

			//Remove resource if "-" button pressed
			if (!resourceStateToRemove.empty())
			{
				RemoveResourceStateFrom(resourceStateToRemove, pResourceStateGroup);
			}

			//Temporal Resource State Group has Output and Input Stages
			if (resourceStateGroupIndex == TEMPORAL_RESOURCE_STATE_GROUP_INDEX)
			{
				imnodes::BeginNode(pResourceStateGroup->InputNodeIndex);

				imnodes::BeginNodeTitleBar();
				ImGui::Text((pResourceStateGroup->Name + "_INPUT").c_str());
				imnodes::EndNodeTitleBar();

				for (const EditorResourceStateIdent& resourceStateIdent : pResourceStateGroup->ResourceStateIdents)
				{
					uint32 primaryAttributeIndex	= resourceStateIdent.AttributeIndex / 2;
					uint32 inputAttributeIndex		= resourceStateIdent.AttributeIndex;
					EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

					PushPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, inputAttributeIndex);
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
					PopPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, inputAttributeIndex);
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
							pResourceStateGroup->ResourceStateIdents.PushBack(CreateResourceState(pResource->Name, pResourceStateGroup->Name, true, ERenderGraphResourceBindingType::NONE));
							m_ParsedGraphDirty = true;
						}
					}

					ImGui::EndDragDropTarget();
				}

				imnodes::EndNode();

				//Remove resource if "-" button pressed
				if (!resourceStateToRemove.empty())
				{
					RemoveResourceStateFrom(resourceStateToRemove, pResourceStateGroup);
				}
			}
		}

		//Final Output
		{
			imnodes::BeginNode(m_FinalOutput.NodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::Text("FINAL_OUTPUT");
			imnodes::EndNodeTitleBar();

			uint32 primaryAttributeIndex	= m_FinalOutput.BackBufferAttributeIndex / 2;
			uint32 inputAttributeIndex		= m_FinalOutput.BackBufferAttributeIndex;
			EditorRenderGraphResourceState* pResource = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

			PushPinColorIfNeeded(EEditorPinType::INPUT, pResource, inputAttributeIndex);
			imnodes::BeginInputAttribute(inputAttributeIndex);
			ImGui::Text(pResource->ResourceName.c_str());
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::INPUT, pResource, inputAttributeIndex);

			imnodes::EndNode();
		}

		//Render Stages
		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStageDesc* pRenderStage = &renderStageIt->second;
			bool hasDepthAttachment = false;

			int32 moveResourceStateAttributeIndex	= -1;
			int32 moveResourceStateMoveAddition		= 0;

			uint8 typeIndex = (static_cast<uint8>(pRenderStage->Type) - 1U); typeIndex = typeIndex < 4U ? typeIndex : 0;
			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,			NODE_TITLE_COLOR[typeIndex]);
			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarHovered,	HOVERED_COLOR[typeIndex]);
			imnodes::PushColorStyle(imnodes::ColorStyle_TitleBarSelected,	SELECTED_COLOR[typeIndex]);

			imnodes::BeginNode(pRenderStage->NodeIndex);

			String renderStageType = RenderStageTypeToString(pRenderStage->Type);

			imnodes::BeginNodeTitleBar();
			ImGui::Text("%s : [%s]", pRenderStage->Name.c_str(), renderStageType.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				renderStageToDelete = pRenderStage->Name;
			}

			int32 selectedTriggerType = TriggerTypeToTriggerTypeIndex(pRenderStage->TriggerType);
			ImGui::Text("Trigger Type: ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::CalcTextSize(TRIGGER_TYPE_NAMES[2]).x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
			if (ImGui::Combo("##Render Stage Trigger Type", &selectedTriggerType, TRIGGER_TYPE_NAMES, 3))
			{
				pRenderStage->TriggerType = TriggerTypeIndexToTriggerType(selectedTriggerType);
			}

			if (pRenderStage->TriggerType == ERenderStageExecutionTrigger::EVERY)
			{
				ImGui::Text("Frame Delay: ");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::CalcTextSize("         ").x + ImGui::GetFrameHeight() * 2 + 4.0f);
				ImGui::InputInt(" ##Render Stage Frame Delay", &pRenderStage->FrameDelay);
				pRenderStage->FrameDelay = glm::clamp<uint32>(pRenderStage->FrameDelay, 0, 360);

				ImGui::Text("Frame Offset: ");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::CalcTextSize("         ").x + ImGui::GetFrameHeight() * 2 + 4.0f);
				ImGui::InputInt("##Render Stage Frame Offset", &pRenderStage->FrameOffset);
				pRenderStage->FrameDelay = glm::clamp<uint32>(pRenderStage->FrameDelay, 0, pRenderStage->FrameDelay);
			}

			ImGui::Text("Allow Overriding of Binding Types:");
			ImGui::SameLine();
			ImGui::Checkbox(("##Override Recommended Binding Types" + pRenderStage->Name).c_str(), &pRenderStage->OverrideRecommendedBindingType);

			imnodes::EndNodeTitleBar();

			String resourceStateToRemove = "";

			//Render Resource State
			for (uint32 resourceStateLocalIndex = 0; resourceStateLocalIndex < pRenderStage->ResourceStateIdents.GetSize(); resourceStateLocalIndex++)
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

				PushPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, inputAttributeIndex);
				imnodes::BeginInputAttribute(inputAttributeIndex);
				ImGui::Text(pResourceState->ResourceName.c_str());
				imnodes::EndInputAttribute();
				PopPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, inputAttributeIndex);

				ImGui::SameLine();

				PushPinColorIfNeeded(EEditorPinType::OUTPUT, pResourceState, outputAttributeIndex);
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

				if (resourceStateLocalIndex < pRenderStage->ResourceStateIdents.GetSize() - 1)
				{
					ImGui::SameLine();

					if (ImGui::ArrowButton("##Move Resource State Down Button", ImGuiDir_Down))
					{
						moveResourceStateAttributeIndex = primaryAttributeIndex;
						moveResourceStateMoveAddition = 1;
					}
				}

				imnodes::EndOutputAttribute();
				PopPinColorIfNeeded(EEditorPinType::OUTPUT, pResourceState, outputAttributeIndex);

				static TArray<ERenderGraphResourceBindingType> bindingTypes;
				static TArray<const char*> bindingTypeNames;
				bindingTypes.Clear();
				bindingTypeNames.Clear();
				CalculateResourceStateBindingTypes(pRenderStage, pResource, pResourceState, bindingTypes, bindingTypeNames);

				if (bindingTypes.GetSize() > 0)
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
						for (int32 bt = 0; bt < int32(bindingTypeNames.GetSize()); bt++)
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

			PushPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, nullptr , -1);
			imnodes::BeginInputAttribute(pRenderStage->InputAttributeIndex);
			ImGui::Text("New Input");
			imnodes::EndInputAttribute();
			PopPinColorIfNeeded(EEditorPinType::RENDER_STAGE_INPUT, nullptr, -1);

			ImGui::Button("Drag Resource Here");

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

				if (pPayload != nullptr)
				{
					RenderGraphResourceDesc* pResource = *reinterpret_cast<RenderGraphResourceDesc**>(pPayload->Data);

					if (pRenderStage->FindResourceStateIdent(pResource->Name) == pRenderStage->ResourceStateIdents.end())
					{
						pRenderStage->ResourceStateIdents.PushBack(CreateResourceState(pResource->Name, pRenderStage->Name, true, ERenderGraphResourceBindingType::NONE));
						m_ParsedGraphDirty = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			//If Graphics, Render Draw Type and Render Pass options
			if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
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

				int32 selectedCullMode = CullModeToCullModeIndex(pRenderStage->Graphics.CullMode);
				ImGui::Text("Cull Mode:");
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(CULL_MODE_NAMES[2]).x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
				if (ImGui::Combo("##Render Stage Cull Mode", &selectedCullMode, CULL_MODE_NAMES, 3))
				{
					pRenderStage->Graphics.CullMode = CullModeIndexToCullMode(selectedCullMode);
				}

				int32 selectedPolygonMode = PolygonModeToPolygonModeIndex(pRenderStage->Graphics.PolygonMode);
				ImGui::Text("Polygon Mode:");
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(POLYGON_MODE_NAMES[2]).x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
				if (ImGui::Combo("##Render Stage Polygon Mode", &selectedPolygonMode, POLYGON_MODE_NAMES, 3))
				{
					pRenderStage->Graphics.PolygonMode = PolygonModeIndexToPolygonMode(selectedPolygonMode);
				}

				int32 selectedPrimitiveTopology = PrimitiveTopologyToPrimitiveTopologyIndex(pRenderStage->Graphics.PrimitiveTopology);
				ImGui::Text("Primitive Topology:");
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(PRIMITIVE_TOPOLOGY_NAMES[0]).x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
				if (ImGui::Combo("##Render Stage Primitive Topology", &selectedPrimitiveTopology, PRIMITIVE_TOPOLOGY_NAMES, 3))
				{
					pRenderStage->Graphics.PrimitiveTopology = PrimitiveTopologyIndexToPrimitiveTopology(selectedPrimitiveTopology);
				}

				TArray<ERenderStageDrawType> drawTypes							= { ERenderStageDrawType::SCENE_INDIRECT, ERenderStageDrawType::FULLSCREEN_QUAD, ERenderStageDrawType::CUBE };
				TArray<const char*> drawTypeNames								= { "SCENE INDIRECT", "FULLSCREEN QUAD", "CUBE" };
				auto selectedDrawTypeIt											= std::find(drawTypes.begin(), drawTypes.end(), pRenderStage->Graphics.DrawType);
				int32 selectedDrawType											= 0;
				if (selectedDrawTypeIt != drawTypes.end()) selectedDrawType		= std::distance(drawTypes.begin(), selectedDrawTypeIt);

				ImGui::Text(("\tDraw Type:"));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::CalcTextSize("FULLSCREEN QUAD").x + ImGui::GetFrameHeight() + 4.0f); //Max Length String to be displayed + Arrow Size + Some extra
				if (ImGui::BeginCombo(("##Draw Type" + pRenderStage->Name).c_str(), drawTypeNames[selectedDrawType]))
				{
					for (int32 dt = 0; dt < int32(drawTypeNames.GetSize()); dt++)
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

						PushPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, -1);
						imnodes::BeginInputAttribute(pRenderStage->Graphics.IndexBufferAttributeIndex);
						ImGui::Text("Index Buffer");

						if (!pResourceState->ResourceName.empty())
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
						PopPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, -1);
					}

					//Indirect Args Buffer
					{
						EditorRenderGraphResourceState* pResourceState = &m_ResourceStatesByHalfAttributeIndex[pRenderStage->Graphics.IndirectArgsBufferAttributeIndex / 2];

						PushPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, -1);
						imnodes::BeginInputAttribute(pRenderStage->Graphics.IndirectArgsBufferAttributeIndex);
						ImGui::Text("Indirect Args Buffer");

						if (!pResourceState->ResourceName.empty())
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
						PopPinColorIfNeeded(EEditorPinType::INPUT, pResourceState, -1);
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
			if (!resourceStateToRemove.empty())
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

			if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
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

			pRenderStage->ResourceStateIdents.Clear();

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
							pRenderStage->ResourceStateIdents.PushBack(CreateResourceState(resourceIt->Name, pRenderStage->Name, true, ERenderGraphResourceBindingType::NONE));
							resourceStateIdentIt = pRenderStage->ResourceStateIdents.begin() + (pRenderStage->ResourceStateIdents.GetSize() - 1);
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

		if		(openNewRenderGraphPopup)	ImGui::OpenPopup("New Render Graph Confirmation ##Popup");
		else if	(openAddRenderStagePopup)	ImGui::OpenPopup("Add Render Stage ##Popup");
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
			if (m_CurrentlyAddingRenderStage != EPipelineStateType::PIPELINE_STATE_TYPE_NONE)
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
					if (m_CurrentlyAddingRenderStage == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
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
					else if (m_CurrentlyAddingRenderStage == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
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
					else if (m_CurrentlyAddingRenderStage == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
					{
						ImGui::Text("Ray Gen. Dimensions");

						ImGuiStyle& style = ImGui::GetStyle();
						static float maxOptionTextSize = style.ItemInnerSpacing.x + ImGui::CalcTextSize(DIMENSION_NAMES[0]).x + ImGui::GetFrameHeight() + 10;

						ImGui::Text("Width: ");
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

					newRenderStage.Parameters.XDimType		= DimensionTypeIndexToDimensionType(selectedXOption);
					newRenderStage.Parameters.YDimType		= DimensionTypeIndexToDimensionType(selectedYOption);
					newRenderStage.Parameters.ZDimType		= DimensionTypeIndexToDimensionType(selectedZOption);

					newRenderStage.Parameters.XDimVariable	= xVariable;
					newRenderStage.Parameters.YDimVariable	= yVariable;
					newRenderStage.Parameters.ZDimVariable	= zVariable;

					s_NextAttributeID += 2;

					if (m_CurrentlyAddingRenderStage == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
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
					m_CurrentlyAddingRenderStage = EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
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
				if (!RenderGraphSerializer::SaveRenderGraphToFile(
					renderGraphNameBuffer,
					m_Resources,
					m_RenderStagesByName,
					m_ResourceStatesByHalfAttributeIndex,
					m_ResourceStateLinksByLinkIndex,
					m_ResourceStateGroups,
					m_FinalOutput))
				{
					LOG_ERROR("[RenderGraphEditor]: Failed to save RenderGraph %s", renderGraphNameBuffer);
				}

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
					renderGraphFilesInDirectory.PushBack(filename.c_str());
				}
			}

			static int32 selectedIndex = 0;
			static bool loadSucceded = true;

			if (selectedIndex >= int32(renderGraphFilesInDirectory.GetSize())) selectedIndex = renderGraphFilesInDirectory.GetSize() - 1;
			ImGui::ListBox("##Render Graph Files", &selectedIndex, renderGraphFilesInDirectory.GetData(), (int32)renderGraphFilesInDirectory.GetSize());

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
				loadSucceded = LoadFromFile(filesInDirectory[selectedIndex]);
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
		links.Clear();
		links.Reserve(m_ParsedRenderGraphStructure.PipelineStageDescriptions.GetSize());

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

				if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
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
				links.PushBack(std::make_tuple(nodeIndex, currentAttributeIndex + 3, currentAttributeIndex + 2));
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
		if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
		{
			if (pRenderStage->Graphics.Shaders.VertexShaderName.empty()		&&
				pRenderStage->Graphics.Shaders.GeometryShaderName.empty()	&&
				pRenderStage->Graphics.Shaders.HullShaderName.empty()		&&
				pRenderStage->Graphics.Shaders.DomainShaderName.empty())
			{
				ImGui::PushID("##Task Shader ID");

				std::filesystem::path taskShaderPath(pRenderStage->Graphics.Shaders.TaskShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.TaskShaderName.empty() ? "Task Shader" : taskShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.TaskShaderName);
				ImGui::PopID();

				ImGui::PushID("##Mesh Shader ID");
				std::filesystem::path meshShaderPath(pRenderStage->Graphics.Shaders.MeshShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.MeshShaderName.empty() ? "Mesh Shader" : meshShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.MeshShaderName);
				ImGui::PopID();
			}

			if (pRenderStage->Graphics.Shaders.TaskShaderName.empty() &&
				pRenderStage->Graphics.Shaders.MeshShaderName.empty())
			{
				ImGui::PushID("##Vertex Shader ID");
				std::filesystem::path vertexShaderPath(pRenderStage->Graphics.Shaders.VertexShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.VertexShaderName.empty() ? "Vertex Shader" : vertexShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.VertexShaderName);
				ImGui::PopID();

				ImGui::PushID("##Geometry Shader ID");
				std::filesystem::path geomentryShaderPath(pRenderStage->Graphics.Shaders.GeometryShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.GeometryShaderName.empty() ? "Geometry Shader" : geomentryShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.GeometryShaderName);
				ImGui::PopID();

				ImGui::PushID("##Hull Shader ID");
				std::filesystem::path hullShaderPath(pRenderStage->Graphics.Shaders.HullShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.HullShaderName.empty() ? "Hull Shader" : hullShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.HullShaderName);
				ImGui::PopID();

				ImGui::PushID("##Domain Shader ID");
				std::filesystem::path domainShaderPath(pRenderStage->Graphics.Shaders.DomainShaderName);
				ImGui::Button(pRenderStage->Graphics.Shaders.DomainShaderName.empty() ? "Domain Shader" : domainShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.DomainShaderName);
				ImGui::PopID();
			}

			ImGui::PushID("##Pixel Shader ID");
			std::filesystem::path pixelShaderPath(pRenderStage->Graphics.Shaders.PixelShaderName);
			ImGui::Button(pRenderStage->Graphics.Shaders.PixelShaderName.empty() ? "Pixel Shader" : pixelShaderPath.filename().string().c_str());
			RenderShaderBoxCommon(&pRenderStage->Graphics.Shaders.PixelShaderName);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE)
		{
			ImGui::PushID("##Compute Shader ID");
			std::filesystem::path computeShaderPath(pRenderStage->Compute.ShaderName);
			ImGui::Button(pRenderStage->Compute.ShaderName.empty() ? "Shader" : computeShaderPath.filename().string().c_str());
			RenderShaderBoxCommon(&pRenderStage->Compute.ShaderName);
			ImGui::PopID();
		}
		else if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING)
		{
			ImGui::PushID("##Raygen Shader ID");
			std::filesystem::path raygenShaderPath(pRenderStage->RayTracing.Shaders.RaygenShaderName);
			ImGui::Button(pRenderStage->RayTracing.Shaders.RaygenShaderName.empty() ? "Raygen Shader" : raygenShaderPath.filename().string().c_str());
			RenderShaderBoxCommon(&pRenderStage->RayTracing.Shaders.RaygenShaderName);
			ImGui::PopID();

			uint32 missBoxesCount = glm::min(pRenderStage->RayTracing.Shaders.MissShaderCount + 1, MAX_MISS_SHADER_COUNT);
			for (uint32 m = 0; m < missBoxesCount; m++)
			{
				bool added = false;
				bool removed = false;

				ImGui::PushID(m);
				std::filesystem::path missShaderPath(pRenderStage->RayTracing.Shaders.pMissShaderNames[m]);
				ImGui::Button(pRenderStage->RayTracing.Shaders.pMissShaderNames[m].empty() ? "Miss Shader" : missShaderPath.filename().string().c_str());
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
				std::filesystem::path closestHitShaderPath(pRenderStage->RayTracing.Shaders.pMissShaderNames[ch]);
				ImGui::Button(pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch].empty() ? "Closest Hit Shader" : closestHitShaderPath.filename().string().c_str());
				RenderShaderBoxCommon(&pRenderStage->RayTracing.Shaders.pClosestHitShaderNames[ch], &added, &removed);
				ImGui::PopID();

				if (added)
					pRenderStage->RayTracing.Shaders.ClosestHitShaderCount++;

				if (removed)
				{
					for (uint32 ch2 = ch; ch2 < closestHitBoxesCount - 1; ch2++)
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
				std::filesystem::path* pShaderName = *reinterpret_cast<std::filesystem::path**>(pPayload->Data);
				if (pAdded != nullptr && pTarget->empty()) (*pAdded) = true;
				(*pTarget) = pShaderName->string();
			}

			ImGui::EndDragDropTarget();
		}

		if (!pTarget->empty())
		{
			ImGui::SameLine();

			if (ImGui::Button("-"))
			{
				(*pTarget) = "";
				if (pRemoved != nullptr) (*pRemoved) = true;
			}
		}
	}

	TArray<RenderGraphResourceDesc>::Iterator RenderGraphEditor::FindResource(const String& name)
	{
		for (auto resourceIt = m_Resources.Begin(); resourceIt != m_Resources.End(); resourceIt++)
		{
			if (resourceIt->Name == name)
				return resourceIt;
		}

		return m_Resources.End();
	}

	EditorResourceStateIdent RenderGraphEditor::CreateResourceState(const String& resourceName, const String& renderStageName, bool removable, ERenderGraphResourceBindingType bindingType)
	{
		if (bindingType == ERenderGraphResourceBindingType::NONE)
		{
			auto resourceIt = FindResource(resourceName);

			if (resourceIt != m_Resources.End())
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
			pResourceStateGroup->ResourceStateIdents.Erase(resourceStateIndexIt);
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
			pRenderStageDesc->ResourceStateIdents.Erase(resourceStateIndexIt);
		}
	}

	void RenderGraphEditor::DestroyLink(int32 linkIndex)
	{
		if (linkIndex >= 0)
		{
			auto linkToBeDestroyedIt = m_ResourceStateLinksByLinkIndex.find(linkIndex);

			if (linkToBeDestroyedIt != m_ResourceStateLinksByLinkIndex.end())
			{
				EditorRenderGraphResourceState* pSrcResource = &m_ResourceStatesByHalfAttributeIndex[linkToBeDestroyedIt->second.SrcAttributeIndex / 2];
				EditorRenderGraphResourceState* pDstResource = &m_ResourceStatesByHalfAttributeIndex[linkToBeDestroyedIt->second.DstAttributeIndex / 2];

				m_ResourceStateLinksByLinkIndex.erase(linkIndex);

				pDstResource->InputLinkIndex = -1;
				pSrcResource->OutputLinkIndices.erase(linkIndex);
				m_ParsedGraphDirty = true;
			}
			else
			{
				LOG_ERROR("[RenderGraphEditor]: DestroyLink called for linkIndex %d which does not exist", linkIndex);
			}
		}
		else
		{
			LOG_ERROR("[RenderGraphEditor]: DestroyLink called for invalid linkIndex %d", linkIndex);
		}
	}

	void RenderGraphEditor::PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pResourceState, targetAttributeIndex))
		{
			imnodes::PushColorStyle(imnodes::ColorStyle_Pin,		HOVERED_COLOR[0]);
			imnodes::PushColorStyle(imnodes::ColorStyle_PinHovered, HOVERED_COLOR[0]);
		}
	}

	void RenderGraphEditor::PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
	{
		if (CustomPinColorNeeded(pinType, pResourceState, targetAttributeIndex))
		{
			imnodes::PopColorStyle();
			imnodes::PopColorStyle();
		}
	}

	bool RenderGraphEditor::CustomPinColorNeeded(EEditorPinType pinType, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex)
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
					if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
					{
						if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
						{
							bindingTypes.PushBack(ERenderGraphResourceBindingType::ATTACHMENT);
							bindingTypeNames.PushBack("ATTACHMENT");
						}
					}

					bindingTypes.PushBack(ERenderGraphResourceBindingType::COMBINED_SAMPLER);
					bindingTypeNames.PushBack("COMBINED SAMPLER");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.PushBack("UNORDERED ACCESS R");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS W");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE TEXTURE
						if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.PushBack(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.PushBack("ATTACHMENT");
							}
						}

						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ TEXTURE

						//If the texture is of Depth/Stencil format we allow Attachment as binding type even if it is discovered as in read mode
						if (pResource->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
						{
							if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
							{
								if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
								{
									bindingTypes.PushBack(ERenderGraphResourceBindingType::ATTACHMENT);
									bindingTypeNames.PushBack("ATTACHMENT");
								}
							}
						}

						bindingTypes.PushBack(ERenderGraphResourceBindingType::COMBINED_SAMPLER);
						bindingTypeNames.PushBack("COMBINED SAMPLER");

						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.PushBack("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE TEXTURE

						if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.PushBack(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.PushBack("ATTACHMENT");
							}
						}

						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS W");
					}
					else if (pResource->TextureParams.TextureFormat == EFormat::FORMAT_D24_UNORM_S8_UINT)
					{
						//If the texture is of Depth/Stencil format we allow Attachment as binding type even if it is discovered as in read mode

						if (pRenderStage->Type == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS)
						{
							if (pResource->TextureParams.IsOfArrayType == false && (pResource->SubResourceCount == 1 || pResource->BackBufferBound))
							{
								bindingTypes.PushBack(ERenderGraphResourceBindingType::ATTACHMENT);
								bindingTypeNames.PushBack("ATTACHMENT");
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
					bindingTypes.PushBack(ERenderGraphResourceBindingType::CONSTANT_BUFFER);
					bindingTypeNames.PushBack("CONSTANT BUFFER");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.PushBack("UNORDERED ACCESS R");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS W");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE BUFFER
						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ BUFFER
						bindingTypes.PushBack(ERenderGraphResourceBindingType::CONSTANT_BUFFER);
						bindingTypeNames.PushBack("CONSTANT BUFFER");

						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.PushBack("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE BUFFER
						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS W");
					}
				}

				break;
			}
			case ERenderGraphResourceType::ACCELERATION_STRUCTURE:
			{
				if (pRenderStage->OverrideRecommendedBindingType)
				{
					bindingTypes.PushBack(ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE);
					bindingTypeNames.PushBack("ACCELERATION STRUCTURE");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
					bindingTypeNames.PushBack("UNORDERED ACCESS R");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS W");

					bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
					bindingTypeNames.PushBack("UNORDERED ACCESS RW");
				}
				else
				{
					if (read && write)
					{
						//READ && WRITE ACCELERATION_STRUCTURE
						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS RW");
					}
					else if (read)
					{
						//READ ACCELERATION_STRUCTURE
						bindingTypes.PushBack(ERenderGraphResourceBindingType::ACCELERATION_STRUCTURE);
						bindingTypeNames.PushBack("ACCELERATION STRUCTURE");

						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_READ);
						bindingTypeNames.PushBack("UNORDERED ACCESS R");
					}
					else if (write)
					{
						//WRITE ACCELERATION_STRUCTURE
						bindingTypes.PushBack(ERenderGraphResourceBindingType::UNORDERED_ACCESS_WRITE);
						bindingTypeNames.PushBack("UNORDERED ACCESS W");
					}
				}

				break;
			}
		}
	}

	bool RenderGraphEditor::LoadFromFile(const String& renderGraphFileName)
	{
		//Reset State
		{
			m_CurrentlyAddingRenderStage	= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
			m_CurrentlyAddingResource		= ERenderGraphResourceType::NONE;

			m_StartedLinkInfo				= {};
		}

		if (!RenderGraphSerializer::LoadFromFile(
			renderGraphFileName,
			m_Resources,
			m_RenderStageNameByInputAttributeIndex,
			m_RenderStagesByName,
			m_ResourceStatesByHalfAttributeIndex,
			m_ResourceStateLinksByLinkIndex,
			m_ResourceStateGroups,
			m_FinalOutput,
			s_NextNodeID,
			s_NextAttributeID,
			s_NextLinkID))
		{
			LOG_ERROR("[RenderGraphEditor]: Failed to load RenderGraph %s from file", renderGraphFileName.c_str());
			return false;
		}

		//Parse the Loaded State
		if (!RenderGraphParser::ParseRenderGraph(
			&m_ParsedRenderGraphStructure,
			m_Resources,
			m_RenderStagesByName,
			m_ResourceStatesByHalfAttributeIndex,
			m_ResourceStateLinksByLinkIndex,
			m_FinalOutput,
			true))
		{
			m_ParsedGraphValid = false;
			LOG_ERROR("[RenderGraphEditor]: Failed to parse RenderGraph %s", renderGraphFileName.c_str());
			return false;
		}

		m_ParsedGraphValid = true;

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
		//Reset to Clear state
		{
			s_NextNodeID		= 0;
			s_NextAttributeID	= 0;
			s_NextLinkID		= 0;

			m_ResourceStateGroups.Clear();
			m_FinalOutput = {};

			m_Resources.Clear();
			m_RenderStageNameByInputAttributeIndex.clear();
			m_RenderStagesByName.clear();
			m_ResourceStatesByHalfAttributeIndex.clear();
			m_ResourceStateLinksByLinkIndex.clear();

			m_CurrentlyAddingRenderStage = EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
			m_CurrentlyAddingResource = ERenderGraphResourceType::NONE;
			m_CurrentlyAddingTextureType = ERenderGraphTextureType::TEXTURE_2D;

			m_StartedLinkInfo = {};

			m_ParsedRenderGraphStructure = {};
		}

		InitDefaultResources();
	}
}