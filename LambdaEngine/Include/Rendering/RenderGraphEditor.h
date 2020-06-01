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
		String				ResourceName	= "";
	};

	struct EditorRenderStage
	{
		String									Name				= "";
		int32									NodeIndex			= 0;
		int32									InputAttributeIndex = 0;
		EPipelineStateType						Type				= EPipelineStateType::NONE;
		bool									CustomRenderer		= false;

		THashTable<String, uint32>				ResourceStates;
	};

	struct EditorExternalResources
	{
		int32									NodeIndex = 0;
		THashTable<String, uint32>				ResourceStates;
	};

	class RenderGraphEditor : public EventHandler
	{
	public:
		DECL_REMOVE_COPY(RenderGraphEditor);
		DECL_REMOVE_MOVE(RenderGraphEditor);

		RenderGraphEditor();
		~RenderGraphEditor();

		void RenderGUI();

		virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat) override final;
		virtual void OnKeyReleased(EKey key)									override final;

	private:
		void InitDefaultResources();

		void RenderResourceView();
		void RenderAddResourceView();
		void RenderGraphView();
		void RenderAddRenderStageView();

		int32 CreateResourceState(String resourceName);
		bool CheckLinkValid(int32* pSrcAttributeIndex, int32* pDstAttributeIndex);

		void FillResourceType(char* pString, const EditorResource* pResource);
		void FillSubResourceType(char* pString, const EditorResource* pResource);
		void FillSubResourceArrayCount(char* pString, const EditorResource* pResource);

	private:
		THashTable<String, EditorResource>					m_ResourcesByName;

		EditorExternalResources								m_ExternalResources;
		THashTable<int32, String>							m_RenderStageNameByInputAttributeIndex;
		THashTable<String, EditorRenderStage>				m_RenderStagesByName;
		THashTable<int32, EditorRenderGraphResourceState>	m_ResourceStatesByHalfAttributeIndex;
		THashTable<int32, EditorRenderGraphResourceLink>	m_ResourceStateLinks;

		EPipelineStateType									m_CurrentlyAddingRenderStage				= EPipelineStateType::NONE;
		EEditorResourceType									m_CurrentlyAddingResource					= EEditorResourceType::NONE;
		String												m_CurrentlyEditedRenderStage				= "";

	private:
		static int32					s_NextNodeID;
		static int32					s_NextAttributeID;
		static int32					s_NextLinkID;
	};
}