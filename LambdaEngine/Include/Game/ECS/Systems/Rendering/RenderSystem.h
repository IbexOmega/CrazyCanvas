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

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/AccelerationStructure.h"

#include "Rendering/RenderGraphTypes.h"
#include "Rendering/LightRenderer.h"

#include "Rendering/ParticleManager.h"
#include "Rendering/RT/ASBuilder.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/GlobalLightProbeComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#define RENDER_SYSTEM_DEBUG 1

namespace LambdaEngine
{
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	// Custom Renderers
	class LineRenderer;
	class PaintMaskRenderer;
	class ParticleRenderer;
	class ParticleUpdater;
	class ParticleCollider;
	class LightRenderer;

	struct CameraComponent;
	struct RenderGraphStructureDesc;
	struct AnimationComponent;
	struct ViewProjectionMatricesComponent;

	class LAMBDA_API RenderSystem : public System
	{
		friend class PaintMaskRenderer;
		DECL_REMOVE_COPY(RenderSystem);
		DECL_REMOVE_MOVE(RenderSystem);

		struct Instance
		{
			glm::mat4	Transform					= glm::mat4(1.0f);
			glm::mat4	PrevTransform				= glm::mat4(1.0f);
			uint32		MaterialIndex				= 0;
			uint32		ExtensionGroupIndex			= 0;
			uint32		TexturesPerExtensionGroup	= 0;
			uint32		MeshletCount				= 0;
		};

		struct MeshKey
		{
		public:
			MeshKey() = default;

			inline MeshKey(GUID_Lambda meshGUID, Entity entityID, bool isAnimated, uint32 entityMask, bool forceUniqueResources)
				: MeshGUID(meshGUID)
				, IsAnimated(isAnimated)
				, ForceUniqueResources(forceUniqueResources)
				, EntityID(entityID)
				, EntityMask(entityMask)
			{
				GetHash();
			}

			size_t GetHash() const
			{
				if (Hash == 0)
				{
					Hash = std::hash<GUID_Lambda>()(MeshGUID);
					HashCombine<GUID_Lambda>(Hash, (GUID_Lambda)EntityMask);
					if (IsAnimated || ForceUniqueResources)
					{
						HashCombine<GUID_Lambda>(Hash, (GUID_Lambda)EntityID);
					}
				}

				return Hash;
			}

			bool operator==(const MeshKey& other) const
			{
				if (MeshGUID != other.MeshGUID || EntityMask != other.EntityMask)
				{
					return false;
				}

				if (IsAnimated)
				{
					if (!other.IsAnimated || EntityID != other.EntityID)
					{
						return false;
					}
				}

				if (ForceUniqueResources && EntityID != other.EntityID)
				{
					return false;
				}

				return true;
			}

		public:
			GUID_Lambda		MeshGUID;
			bool			IsAnimated;
			bool			ForceUniqueResources;
			Entity			EntityID;
			uint32			EntityMask;
			mutable size_t	Hash = 0;
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
			uint32 BLASIndex = BLAS_UNINITIALIZED_INDEX;
			TArray<uint32> ASInstanceIndices;

			DescriptorSet* pAnimationDescriptorSet = nullptr;

			Buffer* pVertexWeightsBuffer	= nullptr;
			Buffer* pAnimatedVertexBuffer	= nullptr;
			Buffer* pVertexBuffer			= nullptr;
			uint32	VertexCount				= 0;
			Buffer* pStagingMatrixBuffer	= nullptr;
			Buffer* pBoneMatrixBuffer		= nullptr;
			uint32	BoneMatrixCount			= 0;
			Buffer* pIndexBuffer			= nullptr;
			uint32	IndexCount				= 0;
			Buffer* pUniqueIndices			= nullptr;
			uint32	UniqueIndexCount		= 0;
			Buffer* pPrimitiveIndices		= nullptr;
			uint32	PrimtiveIndexCount		= 0;
			Buffer* pMeshlets				= nullptr;
			uint32	MeshletCount			= 0;

			TArray<DrawArgExtensionGroup*>	ExtensionGroups;
			bool	HasExtensionData		= false;
			uint32	DrawArgsMask			= 0x0;

			Buffer* pRasterInstanceBuffer				= nullptr;
			Buffer* ppRasterInstanceStagingBuffers[BACK_BUFFER_COUNT];
			TArray<Instance> RasterInstances;

			TArray<Entity> EntityIDs;

			DescriptorSet* pDrawArgDescriptorSet			= nullptr;
			DescriptorSet* pDrawArgDescriptorExtensionsSet	= nullptr;
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

		struct CameraData
		{
			glm::mat4 Projection		= glm::mat4(1.0f);
			glm::mat4 View				= glm::mat4(1.0f);
			glm::mat4 PrevProjection	= glm::mat4(1.0f);
			glm::mat4 PrevView			= glm::mat4(1.0f);
			glm::mat4 ViewInv			= glm::mat4(1.0f);
			glm::mat4 ProjectionInv		= glm::mat4(1.0f);
			glm::vec4 Position			= glm::vec4(0.0f);
			glm::vec4 Right				= glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec4 Up				= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec2 Jitter			= glm::vec2(0.0f);
			glm::vec2 ViewPortSize		= glm::vec2(0.0f);
		};

		struct PerFrameBuffer
		{
			CameraData CamData;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

		struct PointLight
		{
			glm::vec4	ColorIntensity	= glm::vec4(1.0f);
			glm::vec3	Position		= glm::vec3(0.0f);
			float32		NearPlane		= 0.1f;
			float32		FarPlane		= 10.0f;
			uint32		TextureIndex	= 0;
			glm::vec2	padding0		= glm::vec2(0.0f);
			glm::mat4	ProjViews[6];
		};

		struct LightBuffer
		{
			glm::vec4	DirL_ColorIntensity	= glm::vec4(0.0f);
			glm::vec3	DirL_Direction		= glm::vec3(1.0f);
			float32		PointLightCount		= 0;
			glm::mat4	DirL_ProjViews;
			// PointLight PointLights[] unbounded
		};

		struct LightProbe
		{
			inline ~LightProbe()
			{
				Release();
			}

			FORCEINLINE void Release()
			{
				Specular.Reset();
				SpecularView.Reset();

				for (TSharedRef<TextureView>& view : SpecularWriteViews)
				{
					view.Reset();
				}
				RawSpecularWriteViews.Clear();

				Diffuse.Reset();
				DiffuseView.Reset();

				SpecularResolution	= 0;
				DiffuseResolution	= 0;
			}

			TSharedRef<Texture>				Specular;
			TSharedRef<TextureView>			SpecularView;
			TArray<TextureView*>			RawSpecularWriteViews;
			TArray<TSharedRef<TextureView>>	SpecularWriteViews;
			TSharedRef<Texture>		Diffuse;
			TSharedRef<TextureView>	DiffuseView;
			uint32 SpecularResolution	= 128;
			uint32 DiffuseResolution	= 64;
		};

	public:
		~RenderSystem() = default;

		bool Init();
		bool InitRenderGraphs();
		bool Release();

		virtual void Tick(Timestamp deltaTime) override final;

		bool Render(Timestamp delta);

		/*
		* Set new rendergraph to be executed
		*/
		void SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc);

		/*
		* Adds new Game specific Custom Renderer 
		*/
		void AddCustomRenderer(CustomRenderer* pCustomRenderer);

		/*
		* Puts given render stage to sleep, this will prevent execution of renderstage
		* Useful for optimization when a rendergraph can still execute without given renderstage
		*/
		void SetRenderStageSleeping(const String& renderStageName, bool sleeping);

		/*
		* Set Paintmask colors (index 2 -> Team 1 & index 1 -> team 2)
		*/
		void SetPaintMaskColor(uint32 index, const glm::vec3& color);

		RenderGraph*	GetRenderGraph()					{ return m_pRenderGraph;			}
		uint64			GetFrameIndex() const	 			{ return m_FrameIndex;				}
		uint64			GetModFrameIndex() const			{ return m_ModFrameIndex;			}
		uint32			GetBufferIndex() const	 			{ return m_BackBufferIndex;			}
		bool			IsInlineRayTracingEnabled() const	{ return m_InlineRayTracingEnabled; }

	public:
		static RenderSystem& GetInstance() { return s_Instance; }

	private:
		RenderSystem() = default;

		bool InitIntegrationLUT();

		glm::mat4 CreateEntityTransform(Entity entity, const glm::bvec3& rotationalAxes);
		glm::mat4 CreateEntityTransform(
			const PositionComponent& positionComp, 
			const RotationComponent& rotationComp, 
			const ScaleComponent& scaleComp, 
			const glm::bvec3& rotationalAxes);
		
		void OnStaticMeshEntityAdded(Entity entity);
		void OnAnimatedEntityAdded(Entity entity);
		void OnAnimationAttachedEntityAdded(Entity entity);
		void OnPlayerEntityAdded(Entity entity);

		void OnDirectionalEntityAdded(Entity entity);
		void OnDirectionalEntityRemoved(Entity entity);

		void OnPointLightEntityAdded(Entity entity);
		void OnPointLightEntityRemoved(Entity entity);

		void OnGlobalLightProbeEntityAdded(Entity entity);
		void OnGlobalLightProbeEntityRemoved(Entity entity);

		void AddRenderableEntity(
			Entity entity, 
			GUID_Lambda meshGUID, 
			GUID_Lambda materialGUID, 
			const glm::mat4& transform, 
			bool animated, 
			bool forceUniqueResource);

		void RemoveRenderableEntity(Entity entity);

		void OnEmitterEntityRemoved(Entity entity);

		void UpdateParticleEmitter(
			Entity entity, 
			const PositionComponent& positionComp, 
			const RotationComponent& rotationComp, 
			const ParticleEmitterComponent& emitterComp);
		
		void UpdateDirectionalLight(
			const glm::vec4& colorIntensity, 
			const glm::vec3& position, 
			const glm::quat& direction, 
			float frustumWidth, 
			float frustumHeight, 
			float zNear, 
			float zFar);

		void UpdateLightProbeResources(CommandList* pCommandList);
		
		void UpdatePointLight(
			Entity entity, 
			const glm::vec3& position, 
			const glm::vec4& colorIntensity, 
			float nearPlane, 
			float farPlane);
		
		void UpdateAnimation(Entity entity, MeshComponent& meshComp, AnimationComponent& animationComp);
		
		void UpdateTransform(
			Entity entity, 
			const PositionComponent& positionComp, 
			const RotationComponent& rotationComp, 
			const ScaleComponent& scaleComp, 
			const glm::bvec3& rotationalAxes);
		
		void UpdateTransform(
			Entity entity, 
			const glm::mat4& additionalTransform, 
			const PositionComponent& positionComp, 
			const RotationComponent& rotationComp, 
			const ScaleComponent& scaleComp, 
			const glm::bvec3& rotationalAxes);
		
		void UpdateTransformData(Entity entity, const glm::mat4& transform);
		
		void UpdateCamera(
			const glm::vec3& position, 
			const glm::quat& rotation,
			const CameraComponent& camComp, 
			const ViewProjectionMatricesComponent& viewProjComp);

		void DeleteDeviceResource(DeviceChild* pDeviceResource);
		void CleanBuffers();
		void CreateDrawArgs(TArray<DrawArg>& drawArgs, const DrawArgMaskDesc& requestedMaskDesc) const;
		void WriteDrawArgExtensionData(uint32 texturesPerExtensionGroup, MeshEntry& meshEntry);

		void UpdateBuffers();
		void UpdateAnimationBuffers(AnimationComponent& animationComp, MeshEntry& meshEntry);
		void PerformMeshSkinning(CommandList* pCommandList);
		void ExecutePendingBufferUpdates(CommandList* pCommandList);
		void UpdatePerFrameBuffer(CommandList* pCommandList);
		void UpdateRasterInstanceBuffers(CommandList* pCommandList);
		void UpdateMaterialPropertiesBuffer(CommandList* pCommandList);
		void UpdateLightsBuffer(CommandList* pCommandList);
		void UpdatePointLightTextureResource(CommandList* pCommandList);
		void UpdatePaintMaskColorBuffer(CommandList* pCommandList);

		void UpdateRenderGraph();

#ifdef RENDER_SYSTEM_DEBUG
		// Debug
		void CheckWhereEntityAlreadyRegistered(Entity entity);
#endif

	private:
		IDVector m_StaticMeshEntities;
		IDVector m_AnimatedEntities;
		IDVector m_AnimationAttachedEntities;
		IDVector m_LocalPlayerEntities;
		IDVector m_DirectionalLightEntities;
		IDVector m_PointLightEntities;
		IDVector m_CameraEntities;
		IDVector m_ParticleEmitters;
		IDVector m_GlobalLightProbeEntities;

		TSharedRef<SwapChain>	m_SwapChain					= nullptr;
		Texture**				m_ppBackBuffers				= nullptr;
		TextureView**			m_ppBackBufferViews			= nullptr;
		RenderGraph*			m_pRenderGraph				= nullptr;
		uint64					m_FrameIndex				= 0;
		uint64					m_ModFrameIndex				= 0;
		uint32					m_BackBufferIndex			= 0;
		bool					m_RayTracingEnabled			= false;
		bool					m_MeshShadersEnabled		= false;
		bool					m_InlineRayTracingEnabled	= false;

		// Mesh/Instance/Entity
		bool						m_LightsBufferDirty			= true;
		bool						m_PointLightsDirty			= true;
		bool						m_DirectionalExist			= false;
		bool						m_RemoveTexturesOnDeletion	= false;
		TArray<LightUpdateData>		m_PointLightTextureUpdateQueue;
		TArray<uint32>				m_FreeTextureIndices;
		LightBuffer					m_LightBufferData;
		THashTable<Entity, uint32>	m_EntityToPointLight;
		THashTable<uint32, Entity>	m_PointLightToEntity;
		TArray<PointLight>			m_PointLights;
		TArray<glm::vec4>			m_PaintMaskColors;
		TArray<Texture*>			m_CubeTextures;
		TArray<TextureView*>		m_CubeTextureViews;
		TArray<TextureView*>		m_CubeSubImageTextureViews;

		// Global lightprobe, there can only be one global
		LightProbe m_GlobalLightProbe;
		bool m_GlobalLightProbeDirty		= true;
		bool m_GlobalLightProbeNeedsUpdate	= true;

		// Integration Look-Up-Texture
		TSharedRef<Texture>		m_IntegrationLUT;
		TSharedRef<TextureView>	m_IntegrationLUTView;

		// Data Supplied to the RenderGraph
		MeshAndInstancesMap				m_MeshAndInstancesMap;
		MaterialMap						m_MaterialMap;
		THashTable<Entity, InstanceKey> m_EntityIDsToInstanceKey;

		// PAINT_MASK_TEXTURES
		TArray<Texture*>					m_PaintMaskTextures;
		TArray<TextureView*>				m_PaintMaskTextureViews;
		THashTable<uint32, TArray<uint32>>	m_PaintMaskASInstanceIndices;

		// Materials
		TArray<Texture*>			m_AlbedoMaps;
		TArray<Texture*>			m_NormalMaps;
		TArray<Texture*>			m_CombinedMaterialMaps;
		TArray<TextureView*>		m_AlbedoMapViews;
		TArray<TextureView*>		m_NormalMapViews;
		TArray<TextureView*>		m_CombinedMaterialMapViews;
		TArray<MaterialProperties>	m_MaterialProperties;
		TArray<uint32>				m_MaterialInstanceCounts;
		Buffer*						m_ppMaterialParametersStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*						m_pMaterialParametersBuffer = nullptr;
		TArray<uint32>				m_ReleasedMaterialIndices;

		// Per Frame
		PerFrameBuffer		m_PerFrameData;

		Buffer* m_ppLightsStagingBuffer[BACK_BUFFER_COUNT]	= {nullptr};
		Buffer* m_pLightsBuffer								= nullptr;
		Buffer* m_ppPerFrameStagingBuffers[BACK_BUFFER_COUNT];
		Buffer* m_pPerFrameBuffer							= nullptr;
		Buffer* m_ppPaintMaskColorStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*	m_pPaintMaskColorBuffer 					= nullptr;

		// Draw Args
		TSet<DrawArgMaskDesc> m_RequiredDrawArgs;
		
		// Animation
		uint64						m_SkinningPipelineID;
		TSharedRef<PipelineLayout>	m_SkinningPipelineLayout;
		TSharedRef<DescriptorHeap>	m_AnimationDescriptorHeap;

		// Pending/Dirty
		bool						m_MaterialsPropertiesBufferDirty			= false;
		bool						m_MaterialsResourceDirty					= false;
		bool						m_LightsResourceDirty						= false;
		bool						m_PerFrameResourceDirty						= true;
		bool						m_PaintMaskColorsResourceDirty				= true;
		bool						m_RayTracingPaintMaskTexturesResourceDirty	= false;
		TSet<DrawArgMaskDesc>		m_DirtyDrawArgs;
		TSet<MeshEntry*>			m_DirtyRasterInstanceBuffers;
		TSet<MeshEntry*>			m_AnimationsToUpdate;
		TArray<PendingBufferUpdate> m_PendingBufferUpdates;
		TArray<DeviceChild*>		m_ResourcesToRemove[BACK_BUFFER_COUNT];

		// Particles
		ParticleManager				m_ParticleManager;

		// Custom Renderers
		LineRenderer*				m_pLineRenderer			= nullptr;
		LightRenderer*				m_pLightRenderer		= nullptr;
		PaintMaskRenderer*			m_pPaintMaskRenderer	= nullptr;
		ParticleRenderer*			m_pParticleRenderer		= nullptr;
		ParticleUpdater*			m_pParticleUpdater		= nullptr;
		ParticleCollider*			m_pParticleCollider		= nullptr;
		ASBuilder*					m_pASBuilder			= nullptr;
		class LightProbeRenderer*	m_pLightProbeRenderer	= nullptr;
		TArray<CustomRenderer*>		m_GameSpecificCustomRenderers;

#ifdef RENDER_SYSTEM_DEBUG
		// Debug
		TSet<Entity> m_RenderableEntities;
#endif

	private:
		static RenderSystem		s_Instance;
	};
}