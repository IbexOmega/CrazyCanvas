#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Containers/TArray.h"
#include "Containers/String.h"
#include "Camera.h"

#include <map>

namespace LambdaEngine
{
	struct Mesh;
	struct Material;

	class IGraphicsDevice;
	class IAudioDevice;
	class IBuffer;
	class IFence;
	class ITexture;
	class ITextureView;
	class ICommandAllocator;
	class ICommandList;
	class IAccelerationStructure;
	class IDeviceAllocator;
	
	struct GameObject
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};

	struct IndexedIndirectMeshArgument
	{
		uint32	IndexCount			= 0;
		uint32	InstanceCount		= 0;
		uint32	FirstIndex			= 0;
		int32	VertexOffset		= 0;
		uint32	FirstInstance		= 0;
		
		uint32	MaterialIndex		= 0;
	};

	struct SceneDesc
	{
		String	Name				= "Scene";
		bool	RayTracingEnabled	= false;
	};

	struct Instance
	{
		glm::mat3x4 Transform;
		uint32 MeshMaterialIndex : 24;
		uint32 Mask : 8;
		uint32 SBTRecordOffset : 24;
		uint32 Flags : 8;
		uint64 AccelerationStructureAddress;
	};

	class LAMBDA_API Scene
	{
		struct LightsBuffer
		{
			glm::vec4 Direction;
			glm::vec4 EmittedRadiance;
		};

		struct PerFrameBuffer
		{
			CameraData Camera;

			float32 FrameIndex; //We store the Frame Index as a float as it is commonly used as a seed
		};

		struct MappedMaterial
		{
			uint32 MaterialIndex = 0;  //Index to Scene::m_Materials
			std::vector<uint32> InstanceIndices;  //Indices to Scene::m_Instances
		};

		struct MappedMesh
		{
			std::map<GUID_Lambda, uint32>	GUIDToMappedMaterials; //Mapping from GUID to Indices for MappedMesh::MappedMaterials
			std::vector<MappedMaterial>		MappedMaterials;
			uint64							AccelerationStructureHandle;
		};
		
	public:
		DECL_UNIQUE_CLASS(Scene);

		Scene(const IGraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice);
		~Scene();

		bool Init(const SceneDesc& desc);
		bool Finalize();

		void UpdateDirectionalLight(ICommandList* pCommandList, const glm::vec3& direction, const glm::vec3& spectralIntensity);
		void UpdatePerFrameBuffer(ICommandList* pCommandList, const Camera* pCamera, uint64 frameIndex);

		uint32 AddStaticGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		uint32 AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));

		uint32 GetIndirectArgumentOffset(uint32 materialIndex) const;

		//Todo: Make these const
		FORCEINLINE const IAccelerationStructure*	GetTLAS() const					{ return m_pTLAS;}

		FORCEINLINE IBuffer*						GetLightsBuffer()				{ return m_pLightsBuffer; }
		FORCEINLINE IBuffer*						GetPerFrameBuffer()				{ return m_pPerFrameBuffer; }
		FORCEINLINE ITexture**						GetAlbedoMaps()					{ return m_SceneAlbedoMaps.data(); }
		FORCEINLINE ITexture**						GetNormalMaps()					{ return m_SceneNormalMaps.data(); }
		FORCEINLINE ITexture**						GetAmbientOcclusionMaps()		{ return m_SceneAmbientOcclusionMaps.data(); }
		FORCEINLINE ITexture**						GetMetallicMaps()				{ return m_SceneMetallicMaps.data(); }
		FORCEINLINE ITexture**						GetRoughnessMaps()				{ return m_SceneRoughnessMaps.data(); }

		FORCEINLINE ITextureView**					GetAlbedoMapViews()				{ return m_SceneAlbedoMapViews.data(); }
		FORCEINLINE ITextureView**					GetNormalMapViews()				{ return m_SceneNormalMapViews.data(); }
		FORCEINLINE ITextureView**					GetAmbientOcclusionMapViews()	{ return m_SceneAmbientOcclusionMapViews.data(); }
		FORCEINLINE ITextureView**					GetMetallicMapViews()			{ return m_SceneMetallicMapViews.data(); }
		FORCEINLINE ITextureView**					GetRoughnessMapViews()			{ return m_SceneRoughnessMapViews.data(); }

		FORCEINLINE IBuffer*						GetMaterialProperties()			{ return m_pSceneMaterialProperties; }	
		FORCEINLINE IBuffer*						GetVertexBuffer()				{ return m_pSceneVertexBuffer; }		
		FORCEINLINE IBuffer*						GetIndexBuffer()				{ return m_pSceneIndexBuffer; }		
		FORCEINLINE IBuffer*						GetInstanceBufer()				{ return m_pSceneInstanceBuffer; }
		FORCEINLINE IBuffer*						GetIndirectArgsBuffer()			{ return m_pSceneMeshIndexBuffer; }	
																			
		

	private:
		const IGraphicsDevice*						m_pGraphicsDevice;
		const IAudioDevice*							m_pAudioDevice;

		String										m_Name									= "Scene";

		IDeviceAllocator*							m_pDeviceAllocator						= nullptr;

		ICommandAllocator*							m_pCopyCommandAllocator					= nullptr;
		ICommandList*								m_pCopyCommandList						= nullptr;

		ICommandAllocator*							m_pBLASBuildCommandAllocator			= nullptr;
		ICommandAllocator*							m_pTLASBuildCommandAllocator			= nullptr;

		ICommandList*								m_pBLASBuildCommandList					= nullptr;
		ICommandList*								m_pTLASBuildCommandList					= nullptr;

		IFence*										m_pASFence								= nullptr;

		std::map<uint32, uint32>					m_MaterialIndexToIndirectArgOffsetMap;
		std::vector<IndexedIndirectMeshArgument>	m_IndirectArgs;

		IBuffer*									m_pLightsCopyBuffer						= nullptr;
		IBuffer*									m_pPerFrameCopyBuffer					= nullptr;

		IBuffer*									m_pSceneMaterialPropertiesCopyBuffer	= nullptr;
		IBuffer*									m_pSceneVertexCopyBuffer				= nullptr;
		IBuffer*									m_pSceneIndexCopyBuffer					= nullptr;
		IBuffer*									m_pSceneInstanceCopyBuffer				= nullptr;
		IBuffer*									m_pSceneMeshIndexCopyBuffer				= nullptr;

		IBuffer*									m_pLightsBuffer							= nullptr;
		IBuffer*									m_pPerFrameBuffer						= nullptr;

		std::vector<ITexture*>						m_SceneAlbedoMaps;
		std::vector<ITexture*>						m_SceneNormalMaps;
		std::vector<ITexture*>						m_SceneAmbientOcclusionMaps;
		std::vector<ITexture*>						m_SceneMetallicMaps;
		std::vector<ITexture*>						m_SceneRoughnessMaps;

		std::vector<ITextureView*>					m_SceneAlbedoMapViews;
		std::vector<ITextureView*>					m_SceneNormalMapViews;
		std::vector<ITextureView*>					m_SceneAmbientOcclusionMapViews;
		std::vector<ITextureView*>					m_SceneMetallicMapViews;
		std::vector<ITextureView*>					m_SceneRoughnessMapViews;
		
		IBuffer*									m_pSceneMaterialProperties		= nullptr;		//Indexed with result from IndirectMeshArgument::MaterialIndex, contains Scene Material Properties
		IBuffer*									m_pSceneVertexBuffer			= nullptr;			//Indexed with result from Scene::m_pBaseVertexIndexBuffer + Scene::m_pSceneIndexBuffer and contains Scene Vertices
		IBuffer*									m_pSceneIndexBuffer				= nullptr;			//Indexed with result from Scene::m_pMeshIndexBuffer + primitiveID * 3 + triangleCornerID and contains indices to Scene::m_pSceneVertexBuffer
		IBuffer*									m_pSceneInstanceBuffer			= nullptr;			/*Indexed with InstanceID and contains per instance data, we can figure out the InstanceID during shading by using
																						IndirectMeshArgument::BaseInstanceIndex, IndirectMeshArgument::VertexCount and primitiveID <-- Relative to drawID*/

		IBuffer*									m_pSceneMeshIndexBuffer			= nullptr;		/*Indexed with drawID when Shading and contains IndirectMeshArgument structs, primarily:
																						IndirectMeshArgument::FirstIndex		will be used as BaseIndex to m_pSceneIndexBuffer, 
																						IndirectMeshArgument::BaseVertexIndex	will be used as BaseIndex to m_pSceneVertexBuffer,
																						IndirectMeshArgument::MaterialIndex		will be used as MaterialIndex to MaterialBuffers,
																						IndirectMeshArgument::BaseInstanceIndex will be used as BaseIndex to m_pSceneInstanceBuffer*/

		std::map<GUID_Lambda, uint32>				m_GUIDToMappedMeshes;
		std::vector<MappedMesh>						m_MappedMeshes;
		std::vector<const Mesh*>					m_Meshes;
		std::vector<Vertex>							m_SceneVertexArray;
		std::vector<uint32>							m_SceneIndexArray;

		std::map<GUID_Lambda, uint32>				m_GUIDToMaterials;
		std::vector<const Material*>				m_Materials;

		std::vector<Instance>						m_Instances;
		std::vector<Instance>						m_SortedInstances;

		bool										m_RayTracingEnabled			= false;
		IAccelerationStructure*						m_pTLAS						= nullptr;
		std::vector<IAccelerationStructure*>		m_BLASs;
	};
}
