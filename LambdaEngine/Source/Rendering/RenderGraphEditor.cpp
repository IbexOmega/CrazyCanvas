#include "Rendering/RenderGraphEditor.h"

#include "Application/API/CommonApplication.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>

namespace LambdaEngine
{
	int32		RenderGraphEditor::s_NextNodeID				= 0;
	int32		RenderGraphEditor::s_NextAttributeID		= 0;
	int32		RenderGraphEditor::s_NextLinkID				= 0;

	RenderGraphEditor::RenderGraphEditor()
	{
		CommonApplication::Get()->AddEventHandler(this);

		imnodes::StyleColorsDark();

		InitDefaultResources();
	}

	RenderGraphEditor::~RenderGraphEditor()
	{
	}

	void RenderGraphEditor::RenderGUI()
	{
		if (ImGui::Begin("Render Graph Editor"))
		{
			ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
			ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

			float contentRegionWidth	= contentRegionMax.x - contentRegionMin.x;
			float contentRegionHeight	= contentRegionMax.y - contentRegionMin.y;

			if (ImGui::BeginChild("Resource View", ImVec2(contentRegionWidth * 0.3f, 0.0f), false, ImGuiWindowFlags_MenuBar))
			{
				RenderResourceView();
				RenderAddResourceView();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("Graph View", ImVec2(contentRegionWidth * 0.7f, 0.0f), false, ImGuiWindowFlags_MenuBar))
			{
				RenderGraphView();
				RenderAddRenderStageView();
			}
			ImGui::EndChild();
		}
		ImGui::End();
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
		m_ExternalResources.NodeIndex = s_NextNodeID++;

		{
			EditorResource resource = {};
			resource.Name						= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::PER_FRAME;
			resource.SubResourceArrayCount		= 1;
			resource.TextureFormat				= EFormat::FORMAT_B8G8R8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= FULLSCREEN_QUAD_VERTEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= PER_FRAME_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_MAT_PARAM_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_VERTEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_INDEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_INSTANCE_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_MESH_INDEX_BUFFER;
			resource.Type						= EEditorResourceType::BUFFER;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= 1;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_ALBEDO_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_NORMAL_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_AO_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_ROUGHNESS_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}

		{
			EditorResource resource = {};
			resource.Name						= SCENE_METALLIC_MAPS;
			resource.Type						= EEditorResourceType::TEXTURE;
			resource.SubResourceType			= EEditorSubResourceType::ARRAY;
			resource.SubResourceArrayCount		= MAX_UNIQUE_MATERIALS;
			resource.TextureFormat				= EFormat::FORMAT_R8G8B8A8_UNORM;
			m_ResourcesByName[resource.Name]	= resource;

			m_ExternalResources.ResourceStates[resource.Name] = CreateResourceState(resource.Name);
		}
	}

	void RenderGraphEditor::RenderResourceView()
	{
		bool openAddResourcePopup = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				if (ImGui::MenuItem("Add Texture", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::TEXTURE;
				}

				if (ImGui::MenuItem("Add Buffer", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::BUFFER;
				}

				if (ImGui::MenuItem("Add Acceleration Structure", NULL, nullptr))
				{
					openAddResourcePopup = true;
					m_CurrentlyAddingResource = EEditorResourceType::ACCELERATION_STRUCTURE;
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Columns(2);

		static int32 selectedResourceIndex			= -1;
		static EditorResource* pSelectedResource	= nullptr;

		for (auto resourceIt = m_ResourcesByName.begin(); resourceIt != m_ResourcesByName.end(); resourceIt++)
		{
			int32 index = std::distance(m_ResourcesByName.begin(), resourceIt);
			EditorResource* pResource = &resourceIt->second;

			if (ImGui::Selectable(pResource->Name.c_str(), selectedResourceIndex == index))
			{
				selectedResourceIndex	= index;
				pSelectedResource		= pResource;
			}

			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("RESOURCE", &pResource, sizeof(EditorResource*));
				ImGui::EndDragDropSource();
			}
		}

		if (pSelectedResource != nullptr)
		{
			ImGui::NextColumn();

			char buffer[128];

			FillResourceType(buffer, pSelectedResource);
			ImGui::Text("Type: %s", buffer);
			ZERO_MEMORY(buffer, ARR_SIZE(buffer));

			FillSubResourceType(buffer, pSelectedResource);
			ImGui::Text("Sub Resource Type: %s", buffer);
			ZERO_MEMORY(buffer, ARR_SIZE(buffer));

			if (pSelectedResource->SubResourceType == EEditorSubResourceType::ARRAY)
			{
				FillSubResourceArrayCount(buffer, pSelectedResource);
				ImGui::Text("Array Count: %s", buffer);
				ZERO_MEMORY(buffer, ARR_SIZE(buffer));
			}
		}

		if (openAddResourcePopup)
			ImGui::OpenPopup("Add Resource Popup");
	}

	void RenderGraphEditor::RenderAddResourceView()
	{
		constexpr const int32 RESOURCE_NAME_BUFFER_LENGTH = 256;
		static char resourceNameBuffer[RESOURCE_NAME_BUFFER_LENGTH];
		static int32 selectedSubResourceType	= 0;
		static int32 subResourceArrayCount		= 1;

		if (ImGui::BeginPopupModal("Add Resource Popup"))
		{
			if (m_CurrentlyAddingResource != EEditorResourceType::NONE)
			{

				ImGui::AlignTextToFramePadding();

				ImGui::Text("Resource Name:    ");
				ImGui::SameLine();
				ImGui::InputText("##Resource Name", resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				const char* subresourceTypes[2] = { "Array", "Per Frame" };

				ImGui::Text("Sub Resource Type:");
				ImGui::SameLine();
				ImGui::Combo("##Sub Resource Type", &selectedSubResourceType, subresourceTypes, ARR_SIZE(subresourceTypes));

				if (selectedSubResourceType == 0)
				{
					ImGui::Text("Array Count:      ");
					ImGui::SameLine();
					ImGui::SliderInt("##Sub Resource Array Count", &subResourceArrayCount, 1, 128);
				}

				bool done = false;
				bool resourceExists = m_ResourcesByName.find(resourceNameBuffer) != m_ResourcesByName.end();
				bool resourceNameEmpty = resourceNameBuffer[0] == 0;
				bool resourceInvalid = resourceExists || resourceNameEmpty;

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
					EditorResource newResource = {};
					newResource.Name				= resourceNameBuffer;
					newResource.Type				= m_CurrentlyAddingResource;

					if (selectedSubResourceType == 0)
					{
						newResource.SubResourceType			= EEditorSubResourceType::ARRAY;
						newResource.SubResourceArrayCount	= subResourceArrayCount;
					}
					else if (selectedSubResourceType == 1)
					{
						newResource.SubResourceType			= EEditorSubResourceType::PER_FRAME;
						newResource.SubResourceArrayCount	= 1;
					}
						
					m_ResourcesByName[newResource.Name] = newResource;

					done = true;
				}

				if (resourceInvalid)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				if (done)
				{
					ZERO_MEMORY(resourceNameBuffer, RESOURCE_NAME_BUFFER_LENGTH);
					selectedSubResourceType		= 0;
					subResourceArrayCount		= 1;
					m_CurrentlyAddingResource	= EEditorResourceType::NONE;
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

	void RenderGraphEditor::RenderGraphView()
	{
		bool openAddRenderStagePopup = false;
		bool openAddRenderStageResourcePopup = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
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

		static int32	startedAttributeID		= -1;
		static bool		linkStarted				= false;
		static bool		linkStartedOnInputPin	= false;
		static String	linkStartedOnResource	= "";

		imnodes::BeginNodeEditor();

		ImGui::GetWindowDrawList()->Flags &= ~ImDrawListFlags_AntiAliasedLines; //Disable this since otherwise link thickness does not work

		imnodes::BeginNode(m_ExternalResources.NodeIndex);

		imnodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("EXTERNAL_RESOURCES");
		imnodes::EndNodeTitleBar();

		//Render External Resources
		for (auto resourceIt = m_ExternalResources.ResourceStates.begin(); resourceIt != m_ExternalResources.ResourceStates.end(); resourceIt++)
		{
			uint32 primaryAttributeIndex	= resourceIt->second / 2;
			uint32 inputAttributeIndex		= resourceIt->second;
			uint32 outputAttributeIndex		= inputAttributeIndex + 1;
			EditorRenderGraphResourceState* pResource = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

			bool linkablePin = false;

			if (linkStarted)
			{
				if (linkStartedOnResource == pResource->ResourceName || linkStartedOnResource.size() == 0)
				{
					if (linkStartedOnInputPin)
						linkablePin = true;
				}
			}

			if (linkablePin)
			{
				imnodes::PushColorStyle(imnodes::ColorStyle_Pin, IM_COL32(232, 27, 86, 255));
			}

			imnodes::BeginOutputAttribute(outputAttributeIndex);
			ImGui::Text(pResource->ResourceName.c_str());
			imnodes::EndOutputAttribute();

			if (linkablePin)
			{
				imnodes::PopColorStyle();
			}
		}

		imnodes::EndNode();

		for (auto renderStageIt = m_RenderStagesByName.begin(); renderStageIt != m_RenderStagesByName.end(); renderStageIt++)
		{
			EditorRenderStage* pRenderStage = &renderStageIt->second;

			imnodes::BeginNode(pRenderStage->NodeIndex);

			imnodes::BeginNodeTitleBar();
			ImGui::TextUnformatted(pRenderStage->Name.c_str());
			imnodes::EndNodeTitleBar();

			//Render Resources
			for (auto resourceIt = pRenderStage->ResourceStates.begin(); resourceIt != pRenderStage->ResourceStates.end(); resourceIt++)
			{
				uint32 primaryAttributeIndex	= resourceIt->second / 2;
				uint32 inputAttributeIndex		= resourceIt->second;
				uint32 outputAttributeIndex		= inputAttributeIndex + 1;
				EditorRenderGraphResourceState* pResource = &m_ResourceStatesByHalfAttributeIndex[primaryAttributeIndex];

				bool linkablePin = false;

				if (linkStarted)
				{
					if (linkStartedOnResource == pResource->ResourceName || linkStartedOnResource.size() == 0)
					{
						linkablePin = true;
					}
				}

				if (linkablePin && !linkStartedOnInputPin)
				{
					imnodes::PushColorStyle(imnodes::ColorStyle_Pin, IM_COL32(232, 27, 86, 255));
				}

				imnodes::BeginInputAttribute(inputAttributeIndex);
				ImGui::Text(pResource->ResourceName.c_str());
				imnodes::EndInputAttribute();

				if (linkablePin && !linkStartedOnInputPin)
				{
					imnodes::PopColorStyle();
				}

				ImGui::SameLine();

				if (linkablePin && linkStartedOnInputPin)
				{
					imnodes::PushColorStyle(imnodes::ColorStyle_Pin, IM_COL32(232, 27, 86, 255));
				}

				imnodes::BeginOutputAttribute(outputAttributeIndex);
				ImGui::Button("-");
				imnodes::EndOutputAttribute();

				if (linkablePin && linkStartedOnInputPin)
				{
					imnodes::PopColorStyle();
				}
			}

			bool linkablePin = false;

			if (linkStarted)
			{
				if (!linkStartedOnInputPin)
				{
					linkablePin = true;
				}
			}

			if (linkablePin)
			{
				imnodes::PushColorStyle(imnodes::ColorStyle_Pin, IM_COL32(232, 27, 86, 255));
			}

			imnodes::BeginInputAttribute(pRenderStage->InputAttributeIndex);
			ImGui::Text("New Input");
			imnodes::EndInputAttribute();

			if (linkablePin)
			{
				imnodes::PopColorStyle();
			}

			if (ImGui::Button("Add Resource"))
			{
				m_CurrentlyEditedRenderStage		= pRenderStage->Name;
				openAddRenderStageResourcePopup		= true;
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("RESOURCE");

				if (pPayload != nullptr)
				{
					EditorResource* pResource = *reinterpret_cast<EditorResource**>(pPayload->Data);

					if (pRenderStage->ResourceStates.find(pResource->Name) == pRenderStage->ResourceStates.end())
						pRenderStage->ResourceStates[pResource->Name] = CreateResourceState(pResource->Name);
				}

				ImGui::EndDragDropTarget();
			}

			imnodes::EndNode();
		}

		//Render Links
		for (auto linkIt = m_ResourceStateLinks.begin(); linkIt != m_ResourceStateLinks.end(); linkIt++)
		{
			EditorRenderGraphResourceLink* pLink = &linkIt->second;

			imnodes::Link(pLink->LinkIndex, pLink->SrcAttributeIndex, pLink->DstAttributeIndex);
		}

		imnodes::EndNodeEditor();

		startedAttributeID		= -1;
		
		if (imnodes::IsLinkStarted(&startedAttributeID))
		{
			linkStarted				= true;
			linkStartedOnInputPin	= startedAttributeID % 2 == 0;
			linkStartedOnResource	= m_ResourceStatesByHalfAttributeIndex[startedAttributeID / 2].ResourceName;
		}
		
		if (imnodes::IsLinkDropped())
		{
			linkStarted				= false;
			linkStartedOnInputPin	= false;
			linkStartedOnResource	= "";
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
					EditorRenderStage* pRenderStage = &m_RenderStagesByName[renderStageNameIt->second];
					EditorResource* pResource = &m_ResourcesByName[m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2].ResourceName];

					if (pRenderStage->ResourceStates.find(pResource->Name) == pRenderStage->ResourceStates.end())
						pRenderStage->ResourceStates[pResource->Name] = CreateResourceState(pResource->Name);

					dstAttributeIndex = pRenderStage->ResourceStates[pResource->Name];
				}

				if (m_ResourceStatesByHalfAttributeIndex[srcAttributeIndex / 2].ResourceName == m_ResourceStatesByHalfAttributeIndex[dstAttributeIndex / 2].ResourceName)
				{
					EditorRenderGraphResourceLink newLink = {};
					newLink.LinkIndex			= s_NextLinkID++;
					newLink.SrcAttributeIndex	= srcAttributeIndex;
					newLink.DstAttributeIndex	= dstAttributeIndex;
					m_ResourceStateLinks[newLink.LinkIndex] = newLink;
				}
			}

			linkStarted				= false;
			linkStartedOnInputPin	= false;
			linkStartedOnResource	= "";
		}

		//Check for newly destroyed links
		int32 linkIndex = 0;
		if (imnodes::IsLinkDestroyed(&linkIndex))
		{
			m_ResourceStateLinks.erase(linkIndex);

			linkStarted				= false;
			linkStartedOnInputPin	= false;
			linkStartedOnResource	= "";
		}

		if (openAddRenderStagePopup)
			ImGui::OpenPopup("Add Render Stage Popup");
		else if (openAddRenderStageResourcePopup)
			ImGui::OpenPopup("Add Render Stage Resource Popup");
	}

	void RenderGraphEditor::RenderAddRenderStageView()
	{
		constexpr const int32 RENDER_STAGE_NAME_BUFFER_LENGTH = 256;
		static char renderStageNameBuffer[RENDER_STAGE_NAME_BUFFER_LENGTH];
		static bool customRenderer = false;

		if (ImGui::BeginPopupModal("Add Render Stage Popup"))
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
					EditorRenderStage newRenderStage = {};
					newRenderStage.Name					= renderStageNameBuffer;
					newRenderStage.NodeIndex			= s_NextNodeID++;
					newRenderStage.InputAttributeIndex	= s_NextAttributeID;
					newRenderStage.Type					= m_CurrentlyAddingRenderStage;
					newRenderStage.CustomRenderer		= customRenderer;

					s_NextAttributeID += 2;

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

	int32 RenderGraphEditor::CreateResourceState(String resourceName)
	{
		EditorRenderGraphResourceState renderGraphInputResource = {};
		renderGraphInputResource.ResourceName	= resourceName;

		uint32 attributeIndex = s_NextAttributeID;
		s_NextAttributeID += 2;

		m_ResourceStatesByHalfAttributeIndex[attributeIndex / 2] = renderGraphInputResource;
		return attributeIndex;
	}

	bool RenderGraphEditor::CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex)
	{
		int32 src = *pSrcAttributeIndex;
		int32 dst = *pDstAttributeIndex;

		if (src % 2 == 1 && dst % 2 == 0)
		{
			return true;
		}
		else if (src % 2 == 0 && dst % 2 == 1)
		{
			//attributeIndex0 Dst and attributeIndex1 Src
			(*pSrcAttributeIndex) = dst;
			(*pDstAttributeIndex) = src;
			return true;
		}

		return false;
	}

	void RenderGraphEditor::FillResourceType(char* pString, const EditorResource* pResource)
	{
		switch (pResource->Type)
		{
		case EEditorResourceType::TEXTURE:						strcpy(pString, "Texture");					return;
		case EEditorResourceType::BUFFER:						strcpy(pString, "Buffer");					return;
		case EEditorResourceType::ACCELERATION_STRUCTURE:		strcpy(pString, "Acceleration Structure");	return;
		default:												strcpy(pString, "None");					return;
		}
	}

	void RenderGraphEditor::FillSubResourceType(char* pString, const EditorResource* pResource)
	{
		switch (pResource->SubResourceType)
		{
		case EEditorSubResourceType::ARRAY:						strcpy(pString, "Array");		return;
		case EEditorSubResourceType::PER_FRAME:					strcpy(pString, "Per Frame");	return;
		default:												strcpy(pString, "None");		return;
		}
	}

	void RenderGraphEditor::FillSubResourceArrayCount(char* pString, const EditorResource* pResource)
	{
		sprintf(pString, "%d", pResource->SubResourceArrayCount);
	}
}