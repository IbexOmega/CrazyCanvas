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
#include "Rendering/Core/API/AccelerationStructure.h"

#include "Rendering/RenderGraphTypes.h"

#include "Game/Camera.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
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

		struct SBTRecord
		{
			uint64	VertexBufferAddress		= 0;
			uint64	IndexBufferAddress		= 0;
		};

		struct MeshEntry
		{
			AccelerationStructure* pBLAS		= nullptr;
			SBTRecord ShaderRecord			= {};

			Buffer* pVertexBuffer			= nullptr;
			uint32	VertexCount				= 0;
			Buffer* pIndexBuffer			= nullptr;
			uint32	IndexCount				= 0;

			Buffer* pASInstanceBuffer			= nullptr;
			Buffer* ppASInstanceStagingBuffers[BACK_BUFFER_COUNT];
			TArray<AccelerationStructureInstance> ASInstances;

			Buffer* pRasterInstanceBuffer			= nullptr;
			Buffer* ppRasterInstanceStagingBuffers[BACK_BUFFER_COUNT];
			TArray<Instance> RasterInstances;
		};

		struct InstanceKey
		{
			MeshKey MeshKey;
			uint32	InstanceIndex = 0;
		};

		struct PendingBufferUpdate
		{
			Buffer* pSrcBuffer	= nullptr;
			uint64	SrcOffset	= 0;
			Buffer* pDstBuffer	= nullptr;
			uint64	DstOffset	= 0;
			uint64	SizeInBytes	= 0;
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

		bool Init();
		bool Release();

		void Tick(float dt);
		bool Render();

		void SetCamera(const Camera* pCamera);

		CommandList* AcquireGraphicsCopyCommandList();
		CommandList* AcquireComputeCopyCommandList();

		void SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc);

		RenderGraph*	GetRenderGraph()	{ return m_pRenderGraph;	}
		uint64			GetFrameIndex()		{ return m_FrameIndex;		}
		uint64			GetModFrameIndex()	{ return m_ModFrameIndex;	}
		uint32			GetBufferIndex()	{ return m_BackBufferIndex; }

	public:
		static RenderSystem& GetInstance() { return s_Instance; }

	private:
		RenderSystem() = default;

		void OnEntityAdded(Entity entity);
		void OnEntityRemoved(Entity entity);

		void AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool animated);
		void RemoveEntityInstance(Entity entity);

		void UpdateTransform(Entity entity, const glm::mat4& transform);
		void UpdateCamera(Entity entity);

		void CleanBuffers();
		void UpdateBuffers();
		void UpdateRenderGraph();
		void CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const;

		void ExecutePendingBufferUpdates(CommandList* pCommandList);
		void UpdateInstanceBuffers(CommandList* pCommandList);
		void UpdatePerFrameBuffer(CommandList* pCommandList);
		void UpdateMaterialPropertiesBuffer(CommandList* pCommandList);
		void BuildBLASs(CommandList* pCommandList);
		void BuildTLAS(CommandList* pCommandList);

	private:
		IDVector				m_RenderableEntities;
		IDVector				m_CameraEntities;

		TSharedRef<SwapChain>	m_SwapChain			= nullptr;
		Texture**				m_ppBackBuffers		= nullptr;
		TextureView**			m_ppBackBufferViews	= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		uint64					m_FrameIndex		= 0;
		uint64					m_ModFrameIndex		= 0;
		uint32					m_BackBufferIndex	= 0;
		bool					m_RayTracingEnabled	= false;
		//Mesh/Instance/Entity
		MeshAndInstancesMap				m_MeshAndInstancesMap;
		MaterialMap						m_MaterialMap;
		THashTable<Entity, InstanceKey> m_EntityIDsToInstanceKey;

		//Materials
		Texture*			m_ppAlbedoMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppNormalMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppAmbientOcclusionMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppRoughnessMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppMetallicMaps[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppAlbedoMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppNormalMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppAmbientOcclusionMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppRoughnessMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppMetallicMapViews[MAX_UNIQUE_MATERIALS];
		MaterialProperties	m_pMaterialProperties[MAX_UNIQUE_MATERIALS];
		Buffer*				m_ppMaterialParametersStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*				m_pMaterialParametersBuffer				= nullptr;
		TStack<uint32>		m_FreeMaterialSlots;

		//Per Frame
		PerFrameBuffer		m_PerFrameData;
		Buffer*				m_ppPerFrameStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*				m_pPerFrameBuffer			= nullptr;

		//Draw Args
		TSet<uint32>		m_RequiredDrawArgs;

		//Ray Tracing
		Buffer*					m_ppStaticStagingInstanceBuffers[BACK_BUFFER_COUNT];
		Buffer*					m_pCompleteInstanceBuffer		= nullptr;
		uint32					m_MaxInstances					= 0;
		AccelerationStructure*	m_pTLAS							= nullptr;
		TArray<AccelerationStructureGeometryDesc> m_CreateTLASGeometryDescriptions;
		TArray<PendingBufferUpdate> m_CompleteInstanceBufferPendingCopies;

		//Pending/Dirty
		bool						m_MaterialsPropertiesBufferDirty	= true;
		bool						m_MaterialsResourceDirty			= true;
		bool						m_PerFrameResourceDirty				= true;
		TSet<uint32>				m_DirtyDrawArgs;
		TSet<MeshEntry*>			m_DirtyInstanceBuffers;
		TSet<MeshEntry*>			m_DirtyBLASs;
		bool						m_TLASDirty							= true;
		bool						m_TLASResourceDirty					= false;
		TArray<PendingBufferUpdate> m_PendingBufferUpdates;
		TArray<DeviceChild*>		m_ResourcesToRemove[BACK_BUFFER_COUNT];

	private:
		static RenderSystem		s_Instance;
	};
}