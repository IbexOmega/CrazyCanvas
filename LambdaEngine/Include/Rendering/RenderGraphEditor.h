#pragma once

#include "LambdaEngine.h"

#include "RefactoredRenderGraphTypes.h"

#include "Containers/String.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/TSet.h"

#include "Application/API/EventHandler.h"

#include <tsl/ordered_map.h>

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
		
		ERefactoredRenderGraphResourceType	Type					= ERefactoredRenderGraphResourceType::NONE;
		uint32						SubResourceCount			= 1;
		bool						Editable					= false;

		uint32						TextureFormat				= 0;
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
		ERefactoredRenderGraphResourceBindingType BindingType = ERefactoredRenderGraphResourceBindingType::NONE;
		int32			InputLinkIndex		= -1;
		TSet<int32>		OutputLinkIndices;
	};

	struct EditorRenderStageDesc
	{
		String						Name					= "";
		int32						NodeIndex				= 0;
		int32						InputAttributeIndex		= 0;
		EPipelineStateType			Type					= EPipelineStateType::NONE;
		bool						CustomRenderer			= false;
		bool						Enabled					= true;

		struct
		{
			RefactoredGraphicsShaders				Shaders;
			ERefactoredRenderStageDrawType		DrawType;
			int32						IndexBufferAttributeIndex;
			int32						IndirectArgsBufferAttributeIndex;
		} Graphics;

		struct
		{
			String						ShaderName = "";
		} Compute;

		struct
		{
			RefactoredRayTracingShaders			Shaders;
		} RayTracing;

		tsl::ordered_map<String, int32>		ResourceStates;

		uint32						Weight					= 0;
	};

	struct EditorResourceStateGroup
	{
		String								Name			= "";
		int32								NodeIndex		= 0;
		tsl::ordered_map<String, int32>		ResourceStates;
	};

	struct EditorFinalOutput
	{
		String		Name						= "";
		int32		NodeIndex					= 0;
		int32		BackBufferAttributeIndex	= 0;
	};

	class RefactoredRenderGraph;

	class RenderGraphEditor : public EventHandler
	{
	public:
		DECL_REMOVE_COPY(RenderGraphEditor);
		DECL_REMOVE_MOVE(RenderGraphEditor);

		RenderGraphEditor();
		~RenderGraphEditor();

		void InitGUI();
		void RenderGUI();

		RefactoredRenderGraphStructure* CreateRenderGraphStructure(const String& filepath, bool debug);

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

		int32 CreateResourceState(const String& resourceName, const String& renderStageName, bool removable, ERefactoredRenderGraphResourceBindingType bindingType);
		bool CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex);

		String RenderStageTypeToString(EPipelineStateType type);
		String RenderGraphResourceTypeToString(ERefactoredRenderGraphResourceType type);

		EPipelineStateType RenderStageTypeFromString(const String& string);
		ERefactoredRenderGraphResourceType RenderGraphResourceTypeFromString(const String& string);

		void DestroyLink(int32 linkIndex);

		void PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		void PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		bool CustomPinColorNeeded(EEditorPinType pinType, EditorRenderStageDesc* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);

		void CalculateResourceStateBindingTypes(const EditorRenderStageDesc* pRenderStage, const EditorRenderGraphResourceState* pResourceState, TArray<ERefactoredRenderGraphResourceBindingType>& bindingTypes, TArray<const char*>& bindingTypeNames);
		String BindingTypeToShortString(ERefactoredRenderGraphResourceBindingType bindingType);
		String BindingTypeToString(ERefactoredRenderGraphResourceBindingType bindingType);
		ERefactoredRenderGraphResourceBindingType ResourceStateBindingTypeFromString(const String& string);

		bool SaveToFile(const String& renderGraphName);
		bool LoadFromFile(const String& filepath, bool generateImGuiStage);
		void SetInitialNodePositions();

		bool ParseStructure(bool generateImGuiStage);
		bool RecursivelyWeightParentRenderStages(EditorRenderStageDesc* pChildRenderStage);
		bool IsRenderStage(const String& name);
		bool CapturedByImGui(const EditorResource* pResource);
		void FindAndCreateSynchronization(bool generateImGuiStage,
			const std::multimap<uint32, EditorRenderStageDesc*>::reverse_iterator& currentOrderedRenderStageIt, 
			const std::multimap<uint32, EditorRenderStageDesc*>& orderedMappedRenderStages, 
			const EditorRenderGraphResourceState* pCurrentResourceState, 
			RefactoredSynchronizationStageDesc* pSynchronizationStage);
		void CreateParsedRenderStage(RefactoredRenderStageDesc* pDstRenderStage, const EditorRenderStageDesc* pSrcRenderStage);

	private:
		bool												m_GUIInitialized = false;

		TArray<EditorResourceStateGroup>					m_ResourceStateGroups;
		EditorFinalOutput									m_FinalOutput		= {};

		tsl::ordered_map<String, EditorResource>			m_ResourcesByName;

		THashTable<int32, String>							m_RenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStageDesc>				m_RenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	m_ResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	m_ResourceStateLinksByLinkIndex;

		EPipelineStateType									m_CurrentlyAddingRenderStage	= EPipelineStateType::NONE;
		ERefactoredRenderGraphResourceType									m_CurrentlyAddingResource		= ERefactoredRenderGraphResourceType::NONE;
		String												m_CurrentlyEditingResource		= "";

		EditorStartedLinkInfo								m_StartedLinkInfo				= {};

		TArray<String>										m_FilesInShaderDirectory;

		RefactoredRenderGraphStructure								m_ParsedRenderGraphStructure	= {};
		bool												m_ParsedGraphDirty				= true;
		String												m_ParsingError					= "No Errors";

	private:
		static int32					s_NextNodeID;
		static int32					s_NextAttributeID;
		static int32					s_NextLinkID;
	};
}