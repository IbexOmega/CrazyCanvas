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

#include "Rendering/RenderGraphTypes.h"

#include "Game/Camera.h"

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

		struct PendingBufferUpdate
		{
			Buffer* pSrcBuffer	= nullptr;
			Buffer* pDstBuffer	= nullptr;
			uint64 SizeInBytes	= 0;
		};

		using MeshAndInstancesMap	= THashTable<MeshKey, MeshEntry, MeshKeyHasher>;
		using MaterialMap			= THashTable<GUID_Lambda, uint32>;

		struct PerFrameBuffer
		{
			CameraData CamData;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

		struct PointLight
		{
			glm::vec4	ColorIntensity = glm::vec4(1.0f);
			glm::vec3	Position = glm::vec3(0.0f);
			uint32		Padding0;
		};

		struct LightBuffer
		{
			glm::vec4	ColorIntensity	= glm::vec4(1.0f);
			glm::vec3	Direction		= glm::vec3(1.0f);
			uint32		PointLightCount;
			// PointLight PointLights[] unbounded
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

		void OnStaticEntityAdded(Entity entity);
		void OnDynamicEntityAdded(Entity entity);
		void OnDirectionalEntityAdded(Entity entity);
		void OnPointLightEntityAdded(Entity entity);

		void OnDirectionalEntityRemoved(Entity entity);
		void OnPointLightEntityRemoved(Entity entity);

		void RemoveEntityInstance(Entity entity);

		void AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isStatic, bool animated);

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
		void UpdateLightsBuffer(CommandList* pCommandList);

	private:
		IDVector				m_StaticEntities;
		IDVector				m_DynamicEntities;
		IDVector				m_DirectionalLightEntities;
		IDVector				m_PointLightEntities;
		IDVector				m_CameraEntities;

		TSharedRef<SwapChain>	m_SwapChain			= nullptr;
		Texture**				m_ppBackBuffers		= nullptr;
		TextureView**			m_ppBackBufferViews	= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		uint64					m_FrameIndex		= 0;
		uint64					m_ModFrameIndex		= 0;
		uint32					m_BackBufferIndex	= 0;
		bool					m_RayTracingEnabled	= false;

		bool						m_DirtyLights = false;
		bool						m_DirectionalExist = false;
		LightBuffer					m_DirectionalLight;
		THashTable<Entity, uint32>	m_EntityToPointLight;
		THashTable<uint32, Entity>	m_PointLightToEntity;
		TArray<PointLight>			m_PointLights;

		//Data Supplied to the RenderGraph
		MeshAndInstancesMap				m_MeshAndInstancesMap;
		MaterialMap						m_MaterialMap;
		THashTable<Entity, InstanceKey> m_EntityIDsToInstanceKey;

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
		Buffer*				m_pMaterialParametersStagingBuffer		= nullptr;
		Buffer*				m_pMaterialParametersBuffer				= nullptr;
		TStack<uint32>		m_FreeMaterialSlots;

		TSet<MeshEntry*>	m_DirtyInstanceBuffers;
		TArray<Buffer*>		m_BuffersToRemove[BACK_BUFFER_COUNT];
		TArray<PendingBufferUpdate> m_PendingBufferUpdates;

		PerFrameBuffer		m_PerFrameData;
		Buffer*				m_pPerFrameStagingBuffer					= nullptr;
		Buffer*				m_pPerFrameBuffer							= nullptr;

		Buffer*				m_ppLightsStagingBuffer[BACK_BUFFER_COUNT] = {nullptr};
		Buffer*				m_pLightsBuffer								= nullptr;

		TSet<uint32>		m_RequiredDrawArgs;
		TSet<uint32>		m_DirtyDrawArgs;
		bool				m_PerFrameResourceDirty		= true;
		bool				m_MaterialsResourceDirty	= true;

	private:
		static RenderSystem		s_Instance;
	};
}