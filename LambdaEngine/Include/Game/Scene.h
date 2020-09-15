#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Containers/TArray.h"
#include "Containers/String.h"
#include "Containers/TSet.h"
#include "Camera.h"
#include "Time/API/Timestamp.h"

#include <map>

namespace LambdaEngine
{
	constexpr const uint32 MAX_NUM_AREA_LIGHTS	= 4;
	constexpr const uint32 NUM_RANDOM_SEEDS		= 8192;

	struct Mesh;

	class GraphicsDevice;
	class IAudioDevice;
	class Buffer;
	class Texture;
	class TextureView;
	class CommandAllocator;
	class CommandList;
	class AccelerationStructure;
	class Fence;
	class DeviceAllocator;
	
	enum HitMask : uint8
	{
		HIT_MASK_NONE			= 0x00,
		HIT_MASK_GAME_OBJECT	= 0x01,
		HIT_MASK_LIGHT			= 0x02,
		HIT_MASK_ALL			= 0xFF,
	};

	enum EAreaLightType : uint8
	{
		NONE		= 0,
		QUAD		= 1,
	};

	struct SceneDesc
	{
		String	Name				= "Scene";
		bool	RayTracingEnabled	= false;
	};

	struct AreaLightObject
	{
		EAreaLightType	Type;
		GUID_Lambda		Material;
	};

	struct GameObject
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};

	struct InstanceRayTracing
	{
		glm::mat3x4 Transform;
		uint32 CustomIndex : 24;
		uint32 Mask : 8;
		uint32 SBTRecordOffset : 24;
		uint32 Flags : 8;
		uint64 AccelerationStructureAddress;
	};

	struct InstanceRaster
	{
		glm::mat4	Transform		= glm::mat4(1.0f);
		glm::mat4	PrevTransform	= glm::mat4(1.0f);
		uint32		MaterialIndex	= 0;
	};

	struct IndirectArg
	{
		uint32	IndexCount		= 0;
		uint32	InstanceCount	= 0;
		uint32	FirstIndex		= 0;
		int32	VertexOffset	= 0;
		uint32	FirstInstance	= 0;
	};

	struct DirectionalLight
	{
		glm::vec4	Direction;
		glm::vec4	EmittedRadiance;
	};

	struct AreaLight
	{
		uint32		InstanceIndex;
		uint32		Type;
		uint32		Padding0;
		uint32		Padding1;
	};

	class LAMBDA_API Scene
	{
		struct LightSetup
		{
			DirectionalLight	DirectionalLight;
			AreaLight			AreaLights[MAX_NUM_AREA_LIGHTS];
			uint32				AreaLightCount;
		};

		struct PerFrameBuffer
		{
			CameraData Camera;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

		struct MappedMaterial
		{
			uint32 MaterialIndex = 0;  //Index to Scene::m_Materials
			TArray<uint32> InstanceIndices;  //Indices to Scene::m_Instances
		};

		struct MappedMesh
		{
			std::map<GUID_Lambda, uint32>	GUIDToMappedMaterials; //Mapping from GUID to Indices for MappedMesh::MappedMaterials
			TArray<MappedMaterial>			MappedMaterials;
			uint64							AccelerationStructureHandle;
		};
		
	public:
		DECL_UNIQUE_CLASS(Scene);

		Scene(const GraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice);
		~Scene();

		bool Init(const SceneDesc& desc);
		bool Finalize();

		void PrepareRender(CommandList* pGraphicsCommandList, CommandList* pComputeCommandList, uint64 frameIndex);

		uint32 AddStaticGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		uint32 AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		uint32 AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		void UpdateTransform(uint32 instanceIndex, const glm::mat4& transform);

		void SetDirectionalLight(const DirectionalLight& directionalLight);
		uint32 AddAreaLight(const AreaLightObject& lightObject, const glm::mat4& transform = glm::mat4(1.0f));

		void UpdateCamera(const Camera* pCamera);

		void UpdateMaterialProperties(GUID_Lambda materialGUID);

		uint32 GetIndirectArgumentOffset(uint32 materialIndex) const;

		//Todo: Make these const
		FORCEINLINE const AccelerationStructure*	GetTLAS() const					{ return m_pTLAS;}

		FORCEINLINE Buffer*						GetLightsBuffer()				{ return m_pLightsBuffer; }
		FORCEINLINE Buffer*						GetPerFrameBuffer()				{ return m_pPerFrameBuffer; }
		FORCEINLINE Texture**					GetAlbedoMaps()					{ return m_SceneAlbedoMaps.GetData(); }
		FORCEINLINE Texture**					GetNormalMaps()					{ return m_SceneNormalMaps.GetData(); }
		FORCEINLINE Texture**					GetAmbientOcclusionMaps()		{ return m_SceneAmbientOcclusionMaps.GetData(); }
		FORCEINLINE Texture**					GetMetallicMaps()				{ return m_SceneMetallicMaps.GetData(); }
		FORCEINLINE Texture**					GetRoughnessMaps()				{ return m_SceneRoughnessMaps.GetData(); }

		FORCEINLINE TextureView**				GetAlbedoMapViews()				{ return m_SceneAlbedoMapViews.GetData(); }
		FORCEINLINE TextureView**				GetNormalMapViews()				{ return m_SceneNormalMapViews.GetData(); }
		FORCEINLINE TextureView**				GetAmbientOcclusionMapViews()	{ return m_SceneAmbientOcclusionMapViews.GetData(); }
		FORCEINLINE TextureView**				GetMetallicMapViews()			{ return m_SceneMetallicMapViews.GetData(); }
		FORCEINLINE TextureView**				GetRoughnessMapViews()			{ return m_SceneRoughnessMapViews.GetData(); }

		FORCEINLINE Buffer*						GetMaterialProperties()			{ return m_pSceneMaterialProperties; }	
		FORCEINLINE Buffer*						GetVertexBuffer()				{ return m_pSceneVertexBuffer; }		
		FORCEINLINE Buffer*						GetIndexBuffer()				{ return m_pSceneIndexBuffer; }		
		FORCEINLINE Buffer*						GetPrimaryInstanceBuffer()		{ return m_pScenePrimaryInstanceBuffer; }
		FORCEINLINE Buffer*						GetSecondaryInstanceBuffer()	{ return m_pSceneSecondaryInstanceBuffer; }
		FORCEINLINE Buffer*						GetIndirectArgsBuffer()			{ return m_pSceneIndirectArgsBuffer; }	

		FORCEINLINE bool						IsRayTracingEnabled()			{ return m_RayTracingEnabled; }

	private:
		uint32 InternalAddDynamicObject(GUID_Lambda Mesh, GUID_Lambda Material, const glm::mat4& transform, HitMask hitMask);

		void UpdateLightsBuffer(CommandList* pCommandList);
		void UpdatePerFrameBuffer(CommandList* pCommandList);

		void UpdateMaterialPropertiesBuffers(CommandList* pCopyCommandList);
		void UpdateVertexBuffers(CommandList* pCopyCommandList);
		void UpdateIndexBuffers(CommandList* pCopyCommandList);
		void UpdateInstanceBuffers(CommandList* pCopyCommandList);
		void UpdateIndirectArgsBuffers(CommandList* pCopyCommandList);

		void BuildTLAS(CommandList* pBuildCommandList, bool update);

	private:
		const GraphicsDevice*						m_pGraphicsDevice;
		const IAudioDevice*							m_pAudioDevice;

		TArray<

		String										m_Name									= "Scene";

		uint32										m_RandomSeeds[NUM_RANDOM_SEEDS];

		DeviceAllocator*							m_pDeviceAllocator						= nullptr;

		CommandAllocator*							m_pCopyCommandAllocator					= nullptr;
		CommandList*								m_pCopyCommandList						= nullptr;

		CommandAllocator*							m_pBLASBuildCommandAllocator			= nullptr;
		CommandAllocator*							m_pTLASBuildCommandAllocator			= nullptr;

		CommandList*								m_pBLASBuildCommandList					= nullptr;
		CommandList*								m_pTLASBuildCommandList					= nullptr;

		Fence*										m_pASFence								= nullptr;

		std::map<uint32, uint32>					m_MaterialIndexToIndirectArgOffsetMap;
		TArray<IndexedIndirectMeshArgument>			m_IndirectArgs;

		Buffer*										m_pLightsCopyBuffer						= nullptr;
		Buffer*										m_pPerFrameCopyBuffer					= nullptr;

		Buffer*										m_pSceneMaterialPropertiesCopyBuffer	= nullptr;
		Buffer*										m_pSceneVertexCopyBuffer				= nullptr;
		Buffer*										m_pSceneIndexCopyBuffer					= nullptr;
		Buffer*										m_pScenePrimaryInstanceCopyBuffer		= nullptr;
		Buffer*										m_pSceneSecondaryInstanceCopyBuffer		= nullptr;
		Buffer*										m_pSceneIndirectArgsCopyBuffer			= nullptr;


		int32										m_AreaLightIndexToInstanceIndex[MAX_NUM_AREA_LIGHTS];
		LightSetup									m_LightsLightSetup;
		Buffer*										m_pLightsBuffer							= nullptr;

		PerFrameBuffer								m_PerFrameData;
		Buffer*										m_pPerFrameBuffer						= nullptr;

		TArray<Texture*>							m_SceneAlbedoMaps;
		TArray<Texture*>							m_SceneNormalMaps;
		TArray<Texture*>							m_SceneAmbientOcclusionMaps;
		TArray<Texture*>							m_SceneMetallicMaps;
		TArray<Texture*>							m_SceneRoughnessMaps;

		TArray<TextureView*>						m_SceneAlbedoMapViews;
		TArray<TextureView*>						m_SceneNormalMapViews;
		TArray<TextureView*>						m_SceneAmbientOcclusionMapViews;
		TArray<TextureView*>						m_SceneMetallicMapViews;
		TArray<TextureView*>						m_SceneRoughnessMapViews;
			
		TArray<MaterialProperties>					m_SceneMaterialProperties;
		
		Buffer*										m_pSceneMaterialProperties		= nullptr;		//Indexed with result from IndirectMeshArgument::MaterialIndex, contains Scene Material Properties
		Buffer*										m_pSceneVertexBuffer			= nullptr;		//Indexed with result from Scene::m_pBaseVertexIndexBuffer + Scene::m_pSceneIndexBuffer and contains Scene Vertices
		Buffer*										m_pSceneIndexBuffer				= nullptr;		//Indexed with result from Scene::m_pMeshIndexBuffer + primitiveID * 3 + triangleCornerID and contains indices to Scene::m_pSceneVertexBuffer
		Buffer*										m_pScenePrimaryInstanceBuffer	= nullptr;		/*Indexed with InstanceID and contains per instance data (must be same as VkAccelerationStructureInstanceKHR since this buffer is used as instance buffer for ray tracing),
																											we can figure out the InstanceID during shading by using
																											IndexedIndirectMeshArgument::BaseInstanceIndex, IndexedIndirectMeshArgument::VertexCount and primitiveID <-- Relative to drawID*/
		Buffer*										m_pSceneSecondaryInstanceBuffer	= nullptr;		/*Because m_pScenePrimaryInstanceBuffer is used as ray tracing instance buffer we cant fit all per instance data in it, 
																											the rest of the data goes into this buffer instead. This Buffer contains elements of type InstanceSecondary*/
		Buffer*										m_pSceneIndirectArgsBuffer		= nullptr;		/*Indexed with drawID when Shading and contains IndexedIndirectMeshArgument structs, primarily:
																											IndexedIndirectMeshArgument::FirstIndex		will be used as BaseIndex to m_pSceneIndexBuffer, 
																											IndexedIndirectMeshArgument::BaseVertexIndex	will be used as BaseIndex to m_pSceneVertexBuffer,
																											IndexedIndirectMeshArgument::MaterialIndex		will be used as MaterialIndex to MaterialBuffers,
																											IndexedIndirectMeshArgument::BaseInstanceIndex will be used as BaseIndex to m_pSceneInstanceBuffer*/

		std::map<GUID_Lambda, uint32>				m_GUIDToMappedMeshes;
		TArray<MappedMesh>							m_MappedMeshes;
		TArray<const Mesh*>							m_Meshes;
		TArray<Vertex>								m_SceneVertexArray;
		TArray<uint32>								m_SceneIndexArray;

		std::map<GUID_Lambda, uint32>				m_GUIDToMaterials;
		TArray<const Material*>						m_Materials;

		TArray<InstancePrimary>						m_PrimaryInstances;
		TArray<InstanceSecondary>					m_SecondaryInstances;
		TArray<InstancePrimary>						m_SortedPrimaryInstances;
		TArray<InstanceSecondary>					m_SortedSecondaryInstances;
		TArray<uint32>								m_InstanceIndexToSortedInstanceIndex;
		TSet<uint32>								m_DirtySecondaryInstances;

		bool										m_RayTracingEnabled			= false;
		AccelerationStructure*						m_pTLAS						= nullptr;
		TArray<AccelerationStructure*>				m_BLASs;

		bool										m_LightSetupIsDirty					= false;
		bool										m_InstanceBuffersAreDirty			= false;
		bool										m_MaterialPropertiesBuffersAreDirty	= false;
	};
}
