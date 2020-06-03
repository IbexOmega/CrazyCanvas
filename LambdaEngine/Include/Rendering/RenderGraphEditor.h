#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"

#include "Containers/String.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/TSet.h"

#include "Application/API/EventHandler.h"

#include <tsl/ordered_map.h>

namespace LambdaEngine
{
	enum class EEditorResourceType : uint8
	{
		NONE					= 0,
		TEXTURE					= 1,
		BUFFER					= 2,
		ACCELERATION_STRUCTURE	= 3,
	};

	enum class EEditorSubResourceType : uint8
	{
		NONE					= 0,
		ARRAY					= 1,
		PER_FRAME				= 2,
	};

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
		
		EEditorResourceType			Type					= EEditorResourceType::NONE;
		EEditorSubResourceType		SubResourceType			= EEditorSubResourceType::NONE;
		uint32						SubResourceArrayCount	= 1;

		EFormat						TextureFormat			= EFormat::NONE; //Todo: How to solve?
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
		bool			Removable			= true;
		int32			InputLinkIndex		= -1;
		TSet<int32>		OutputLinkIndices;
	};

	struct EditorGraphicsShaders
	{
		String TaskShader		= "";
		String MeshShader		= "";

		String VertexShader		= "";
		String GeometryShader	= "";
		String HullShader		= "";
		String DomainShader		= "";

		String PixelShader		= "";
	};

	struct EditorComputeShader
	{
		String Shader			= "";
	};

	struct EditorRayTracingShaders
	{
		String			RaygenShader			= "";
		String			pMissShaders[MAX_MISS_SHADER_COUNT];
		String			pClosestHitShaders[MAX_CLOSEST_HIT_SHADER_COUNT];
		uint32			MissShaderCount			= 0;
		uint32			ClosestHitShaderCount	= 0;
	};

	struct EditorRenderStage
	{
		String						Name					= "";
		int32						NodeIndex				= 0;
		int32						InputAttributeIndex		= 0;
		EPipelineStateType			Type					= EPipelineStateType::NONE;
		bool						CustomRenderer			= false;

		EditorGraphicsShaders		GraphicsShaders;
		EditorComputeShader			ComputeShaders;
		EditorRayTracingShaders		RayTracingShaders;

		tsl::ordered_map<String, int32>		ResourceStates;	
	};

	struct EditorResourceStateGroup
	{
		String								Name			= "";
		int32								NodeIndex		= 0;
		tsl::ordered_map<String, int32>		ResourceStates;
	};

	struct EditorFinalOutput
	{
		int32		NodeIndex					= 0;
		int32		BackBufferAttributeIndex	= 0;
	};

	class RenderGraphEditor : public EventHandler
	{
	public:
		DECL_REMOVE_COPY(RenderGraphEditor);
		DECL_REMOVE_MOVE(RenderGraphEditor);

		RenderGraphEditor();
		~RenderGraphEditor();

		void RenderGUI();

		virtual void OnButtonReleased(EMouseButton button)						override final;
		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat) override final;
		virtual void OnKeyReleased(EKey key)									override final;

	private:
		void InitDefaultResources();

		void RenderResourceView();
		void RenderAddResourceView();
		void RenderShaderView();
		void RenderGraphView();
		void RenderAddRenderStageView();
		void RenderSaveRenderGraphView();
		void RenderLoadRenderGraphView();

		void RenderShaderBoxes(EditorRenderStage* pRenderStage);
		void RenderShaderBoxCommon(String* pTarget, bool* pAdded = nullptr, bool* pRemoved = nullptr);

		int32 CreateResourceState(const String& resourceName, bool removable);
		bool CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex);

		String RenderStageTypeToString(EPipelineStateType type);
		String RenderGraphResourceTypeToString(EEditorResourceType type);
		String RenderGraphSubResourceTypeToString(EEditorSubResourceType type);

		EPipelineStateType RenderStageTypeFromString(const String& string);
		EEditorResourceType RenderGraphResourceTypeFromString(const String& string);
		EEditorSubResourceType RenderGraphSubResourceTypeFromString(const String& string);

		void DestroyLink(int32 linkIndex);

		void PushPinColorIfNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		void PopPinColorIfNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);
		bool CustomPinColorNeeded(EEditorPinType pinType, EditorRenderStage* pRenderStage, EditorRenderGraphResourceState* pResourceState, int32 targetAttributeIndex);

		bool SaveToFile(const String& renderGraphName);
		bool LoadFromFile(const String& filepath);

	private:
		TArray<EditorResourceStateGroup>					m_ResourceStateGroups;
		EditorFinalOutput									m_FinalOutput		= {};

		THashTable<String, EditorResource>					m_ResourcesByName;

		THashTable<int32, String>							m_RenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStage>				m_RenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	m_ResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	m_ResourceStateLinks;

		EPipelineStateType									m_CurrentlyAddingRenderStage				= EPipelineStateType::NONE;
		EEditorResourceType									m_CurrentlyAddingResource					= EEditorResourceType::NONE;

		EditorStartedLinkInfo								m_StartedLinkInfo							= {};

		TArray<String>										m_FilesInShaderDirectory;

	private:
		static int32					s_NextNodeID;
		static int32					s_NextAttributeID;
		static int32					s_NextLinkID;
	};
}