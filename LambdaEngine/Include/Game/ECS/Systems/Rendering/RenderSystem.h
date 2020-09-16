#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"

#include "Utilities/HashUtilities.h"

#include "Containers/String.h"
#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/TStack.h"
#include "Containers/IDVector.h"

#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"

#include "Game/Camera.h"

namespace LambdaEngine
{
	class Scene;
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;

	struct RenderGraphStructureDesc;

	class LAMBDA_API RenderSystem : public System
	{
		struct Instance
		{
			glm::mat4	Transform		= glm::mat4(1.0f);
			glm::mat4	PrevTransform	= glm::mat4(1.0f);
			uint32		MaterialSlot	= 0;
			uint32		Padding0;
			uint32		Padding1;
			uint32		Padding2;
		};

		struct MeshKey
		{
			GUID_Lambda		MeshGUID;
			bool			IsStatic;
			bool			IsAnimated;
			Entity			EntityID;
			mutable size_t	Hash = 0;

			size_t GetHash() const
			{
				if (Hash == 0)
				{
					Hash = std::hash<GUID_Lambda>()(MeshGUID);

					if (IsAnimated) HashCombine<GUID_Lambda>(Hash, (GUID_Lambda)EntityID);
				}

				return Hash;
			}

			bool operator==(const MeshKey& other) const
			{
				if (MeshGUID != other.MeshGUID)
					return false;

				if (IsStatic != other.IsStatic)
					return false;

				if (IsAnimated)
				{
					if (!other.IsAnimated || EntityID != other.EntityID)
						return false;
				}
				
				return true;
			}
		};

		struct MeshKeyHasher
		{
			size_t operator()(const MeshKey& key) const
			{
				return key.GetHash();
			}
		};

		struct MeshEntry
		{
			Buffer* pVertexBuffer			= nullptr;
			Buffer* pIndexBuffer			= nullptr;
			uint32	IndexCount				= 0;

			Buffer* pInstanceBuffer			= nullptr;
			Buffer* pInstanceStagingBuffer	= nullptr;
			TArray<Instance> Instances;
		};

		struct InstanceKey
		{
			MeshKey MeshKey;
			uint32	InstanceIndex = 0;
		};

		using MeshAndInstancesMap	= THashTable<MeshKey, MeshEntry, MeshKeyHasher>;
		using MaterialMap			= THashTable<GUID_Lambda, uint32>;

		struct PerFrameBuffer
		{
			CameraData CamData;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

	public:
		DECL_REMOVE_COPY(RenderSystem);
		DECL_REMOVE_MOVE(RenderSystem);
		~RenderSystem() = default;

		bool InitSystem();

		bool Init();

		bool Release();

		void Tick(float dt);

		bool Render();

		void SetCamera(const Camera* pCamera);

		CommandList* AcquireGraphicsCopyCommandList();
		CommandList* AcquireComputeCopyCommandList();	

		void SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc);

		RenderGraph*	GetRenderGraph()	{ return m_pRenderGraph; }
		uint64			GetFrameIndex()		{ return m_FrameIndex; }
		uint64			GetModFrameIndex()	{ return m_ModFrameIndex; }
		uint32			GetBufferIndex()	{ return m_BackBufferIndex; }
	public:
		static RenderSystem& GetInstance() { return s_Instance; }

	private:
		RenderSystem() = default;

		void AddEntityInstance(Entity entity, CommandList* pCommandList, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isStatic, bool animated);
		
		void UpdateTransform(Entity entity, const glm::mat4& transform);
		void UpdateCamera(Entity entity);

		void UpdateInstanceBuffers(CommandList* pCommandList, uint64 modFrameIndex);
		void UpdatePerFrameBuffer(CommandList* pCommandList);
		void UpdateMaterialPropertiesBuffer(CommandList* pCommandList, uint64 modFrameIndex);

		void UpdateRenderGraphFromScene();

	private:
		IDVector				m_StaticEntities;
		IDVector				m_DynamicEntities;
		IDVector				m_CameraEntities;

		TSharedRef<SwapChain>	m_SwapChain			= nullptr;
		Texture**				m_ppBackBuffers		= nullptr;
		TextureView**			m_ppBackBufferViews	= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		Scene*					m_pScene			= nullptr;
		uint64					m_FrameIndex		= 0;
		uint64					m_ModFrameIndex		= 0;
		uint32					m_BackBufferIndex	= 0;

		MeshAndInstancesMap	m_MeshAndInstancesMap;
		MaterialMap			m_MaterialMap;

		Texture*			m_ppSceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneNormalMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneAmbientOcclusionMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneRoughnessMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneMetallicMaps[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneAlbedoMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneNormalMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneAmbientOcclusionMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneRoughnessMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneMetallicMapViews[MAX_UNIQUE_MATERIALS];
		MaterialProperties	m_pSceneMaterialProperties[MAX_UNIQUE_MATERIALS];
		Buffer*				m_pMaterialParametersStagingBuffer		= nullptr;
		Buffer*				m_pMaterialParametersBuffer				= nullptr;
		TStack<uint32>		m_FreeMaterialSlots;

		TSet<MeshEntry*>	m_DirtyInstanceBuffers;

		TArray<Buffer*>		m_BuffersToRemove[BACK_BUFFER_COUNT];

		THashTable<Entity, InstanceKey> m_EntityIDsToInstanceKey;

		PerFrameBuffer	m_PerFrameData;
		Buffer*			m_pPerFrameStagingBuffer	= nullptr;
		Buffer*			m_pPerFrameBuffer			= nullptr;

		bool							m_RayTracingEnabled		= false;
		AccelerationStructure*			m_pTLAS					= nullptr;
		TArray<AccelerationStructure*>	m_BLASs;

	private:
		static RenderSystem		s_Instance;
	};
}