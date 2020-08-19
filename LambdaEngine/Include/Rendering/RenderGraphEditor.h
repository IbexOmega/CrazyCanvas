#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"

#include "Containers/String.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/TSet.h"

#include "Application/API/EventHandler.h"

namespace LambdaEngine
{
	enum class EEditorPinType : uint8
	{
		INPUT					= 0,
		OUTPUT					= 1,
		RENDER_STAGE_INPUT		= 2,
	};

	struct EditorStartedLinkInfo
	{
		bool		LinkStarted				= false;
		int32		LinkStartAttributeID	= -1;
		bool		LinkStartedOnInputPin	= false;
		String		LinkStartedOnResource	= "";
	};

	struct EditorResource 
	{
		String						Name					= "";
		
		ERenderGraphResourceType	Type					= ERenderGraphResourceType::NONE;
		bool						BackBufferBound			= false;
		uint32						SubResourceCount		= 1;
		bool						IsOfArrayType			= false;
		bool						Editable				= false;

		uint32						TextureFormat			= 0;
	};

	struct EditorRenderGraphResourceLink
	{
		int32		LinkIndex				= 0;
		int32		SrcAttributeIndex		= 0;
		int32		DstAttributeIndex		= 0;
	};

	struct EditorRenderGraphResourceState
	{
		String			ResourceName		= "";
		String			RenderStageName		= "";
		bool			Removable			= true;
		ERenderGraphResourceBindingType BindingType = ERenderGraphResourceBindingType::NONE;
		int32			InputLinkIndex		= -1;
		TSet<int32>		OutputLinkIndices;
	};

	struct EditorResourceStateIdent
	{
		String	Name			= "";
		int32	AttributeIndex	= -1;
	};

	struct EditorRenderStageDesc
	{
		String						Name					= "";
		int32						NodeIndex				= 0;
		int32						InputAttributeIndex		= 0;
		EPipelineStateType			Type					= EPipelineStateType::NONE;
		bool						CustomRenderer			= false;
		bool						Enabled					= true;
		RenderStageParameters		Parameters				= {};

		struct
		{
			GraphicsShaderNames			Shaders;
			ERenderStageDrawType		DrawType;
			int32						IndexBufferAttributeIndex;
			int32						IndirectArgsBufferAttributeIndex;

		} Graphics;

		struct
		{
			String						ShaderName = "";
		} Compute;

		struct
		{
			RayTracingShaderNames		Shaders;
		} RayTracing;

		uint32							Weight					= 0;

		TArray<EditorResourceStateIdent>		ResourceStateIdents;

		TArray<EditorResourceStateIdent>::iterator FindResourceStateIdent(const String& name)
		{
			for (auto resourceStateIt = ResourceStateIdents.begin(); resourceStateIt != ResourceStateIdents.end(); resourceStateIt++)
			{
				if (resourceStateIt->Name == name)
					return resourceStateIt;
			}

			return ResourceStateIdents.end();
		}

		TArray<EditorResourceStateIdent>::const_iterator FindResourceStateIdent(const String& name) const
		{
			for (auto resourceStateIt = ResourceStateIdents.begin(); resourceStateIt != ResourceStateIdents.end(); resourceStateIt++)
			{
				if (resourceStateIt->Name == name)
					return resourceStateIt;
			}

			return ResourceStateIdents.end();
		}
	};

	struct EditorResourceStateGroup
	{
		String								Name				= "";
		int32								InputNodeIndex		= 0;
		int32								OutputNodeIndex		= 0;
		TArray<EditorResourceStateIdent>	ResourceStateIdents;

		TArray<EditorResourceStateIdent>::iterator FindResourceStateIdent(const String& name)
		{
			for (auto resourceStateIt = ResourceStateIdents.begin(); resourceStateIt != ResourceStateIdents.end(); resourceStateIt++)
			{
				if (resourceStateIt->Name == name)
					return resourceStateIt;
			}

			return ResourceStateIdents.end();
		}

		TArray<EditorResourceStateIdent>::const_iterator FindResourceStateIdent(const String& name) const
		{
			for (auto resourceStateIt = ResourceStateIdents.begin(); resourceStateIt != ResourceStateIdents.end(); resourceStateIt++)
			{
				if (resourceStateIt->Name == name)
					return resourceStateIt;
			}

			return ResourceStateIdents.end();
		}
	};

	struct EditorFinalOutput
	{
		String		Name						= "";
		int32		NodeIndex					= 0;
		int32		BackBufferAttributeIndex	= 0;
	};

	class RenderGraph;

	class RenderGraphEditor : public EventHandler
	{
	public:
		DECL_REMOVE_COPY(RenderGraphEditor);
		DECL_REMOVE_MOVE(RenderGraphEditor);

		RenderGraphEditor();
		~RenderGraphEditor();

		void InitGUI();
		void RenderGUI();

		RenderGraphStructureDesc CreateRenderGraphStructure(const String& filepath, bool imGuiEnabled);

		virtual void OnButtonReleased(EMouseButton button)						override final;
		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat) override final;
		virtual void OnKeyReleased(EKey key)									override final;

	private:
		void InitDefaultResources();

		void RenderResourceView(float textWidth, float textHeight);
		void RenderAddResourceView();
		void RenderEditResourceView();

		void RenderShaderView(float textWidth, float textHeight);

		void RenderGraphView();
		void RenderAddRenderStageView();
		void RenderSaveRenderGraphView();
		void RenderLoadRenderGraphView();

		void RenderParsedRenderGraphView();

		void RenderShaderBoxes(EditorRenderStageDesc* pRenderStage);
		void RenderShaderBoxCommon(String* pTarget, bool* pAdded = nullptr, bool* pRemoved = nullptr);

		TArray<EditorResource>::iterator FindResource(const String& name);
		EditorResourceStateIdent CreateResourceState(const String& resourceName, const String& renderStageName, bool removable, ERenderGraphResourceBindingType bindingType);
		bool CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex);

		void DestroyLink(int32 linkIndex);

		void PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		void PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		bool CustomPinColorNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);

		void CalculateResourceStateBindingTypes(const EditorRenderStageDesc* pRenderStage, const EditorRenderGraphResourceState* pResourceState, TArray<ERenderGraphResourceBindingType>& bindingTypes, TArray<const char*>& bindingTypeNames);

		bool SaveToFile(const String& renderGraphName);
		bool LoadFromFile(const String& filepath, bool generateImGuiStage);
		void SetInitialNodePositions();
		void ResetState();

		bool ParseStructure(bool generateImGuiStage);
		bool RecursivelyWeightParentRenderStages(EditorRenderStageDesc* pChildRenderStage);
		bool IsRenderStage(const String& name);
		bool CapturedByImGui(const EditorResource* pResource);
		bool FindAndCreateSynchronization(bool generateImGuiStage,
			const std::multimap<uint32, EditorRenderStageDesc*>::reverse_iterator& currentOrderedRenderStageIt, 
			const std::multimap<uint32, EditorRenderStageDesc*>& orderedMappedRenderStages, 
			const EditorRenderGraphResourceState* pCurrentResourceState, 
			SynchronizationStageDesc* pSynchronizationStage);
		void CreateParsedRenderStage(RenderStageDesc* pDstRenderStage, const EditorRenderStageDesc* pSrcRenderStage);

	private:
		bool												m_GUIInitialized = false;

		TArray<EditorResourceStateGroup>					m_ResourceStateGroups;
		EditorFinalOutput									m_FinalOutput		= {};

		TArray<EditorResource>								m_Resources;

		THashTable<int32, String>							m_RenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStageDesc>			m_RenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	m_ResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	m_ResourceStateLinksByLinkIndex;

		EPipelineStateType									m_CurrentlyAddingRenderStage	= EPipelineStateType::NONE;
		ERenderGraphResourceType							m_CurrentlyAddingResource		= ERenderGraphResourceType::NONE;
		String												m_CurrentlyEditingResource		= "";

		EditorStartedLinkInfo								m_StartedLinkInfo				= {};

		TArray<String>										m_FilesInShaderDirectory;

		RenderGraphStructureDesc							m_ParsedRenderGraphStructure	= {};
		bool												m_ParsedGraphDirty				= true;
		bool												m_ParsedGraphRenderDirty		= true;
		String												m_ParsingError					= "No Errors";

	private:
		static int32					s_NextNodeID;
		static int32					s_NextAttributeID;
		static int32					s_NextLinkID;
	};
}