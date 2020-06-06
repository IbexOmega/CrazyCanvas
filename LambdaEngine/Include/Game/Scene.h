#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Containers/TArray.h"
#include "Camera.h"

#include <map>

namespace LambdaEngine
{
	struct Mesh;
	struct Material;

	class GraphicsDevice;
	class IAudioDevice;
	class Buffer;
	class Texture;
	class TextureView;
	class CommandAllocator;
	class CommandList;
	class AccelerationStructure;
	
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
		const char* pName					= "Scene";
		bool RayTracingEnabled				= false;
	};

	class LAMBDA_API Scene
	{
		struct LightsBuffer
		{
			glm::vec4 Direction;
			glm::vec4 SpectralIntensity;
		};

		struct PerFrameBuffer
		{
			CameraData Camera;
		};

		struct Instance
		{
			glm::mat3x4 Transform;
			uint32 MeshMaterialIndex	: 24;
			uint32 Mask					: 8;
			uint32 SBTRecordOffset		: 24;
			uint32 Flags				: 8;
			uint64 AccelerationStructureHandle;
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

		Scene(const GraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice);
		~Scene();

		bool Init(const SceneDesc& desc);
		bool Finalize();

		void UpdateDirectionalLight(const glm::vec3& direction, const glm::vec3& spectralIntensity);
		void UpdateCamera(const Camera* pCamera);

		uint32 AddStaticGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		uint32 AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));

		uint32 GetIndirectArgumentOffset(uint32 materialIndex) const;

		//Todo: Make these const
		FORCEINLINE AccelerationStructure*	GetTLAS()						{ return m_pTLAS;}

		FORCEINLINE Buffer*				GetLightsBuffer()				{ return m_pLightsBuffer; }
		FORCEINLINE Buffer*				GetPerFrameBuffer()				{ return m_pPerFrameBuffer; }
		FORCEINLINE Texture**				GetAlbedoMaps()					{ return m_SceneAlbedoMaps.data(); }			
		FORCEINLINE Texture**				GetNormalMaps()					{ return m_SceneNormalMaps.data(); }
		FORCEINLINE Texture**				GetAmbientOcclusionMaps()		{ return m_SceneAmbientOcclusionMaps.data(); }
		FORCEINLINE Texture**				GetMetallicMaps()				{ return m_SceneMetallicMaps.data(); }
		FORCEINLINE Texture**				GetRoughnessMaps()				{ return m_SceneRoughnessMaps.data(); }

		FORCEINLINE TextureView**			GetAlbedoMapViews()				{ return m_SceneAlbedoMapViews.data(); }
		FORCEINLINE TextureView**			GetNormalMapViews()				{ return m_SceneNormalMapViews.data(); }
		FORCEINLINE TextureView**			GetAmbientOcclusionMapViews()	{ return m_SceneAmbientOcclusionMapViews.data(); }
		FORCEINLINE TextureView**			GetMetallicMapViews()			{ return m_SceneMetallicMapViews.data(); }
		FORCEINLINE TextureView**			GetRoughnessMapViews()			{ return m_SceneRoughnessMapViews.data(); }

		FORCEINLINE Buffer*				GetMaterialProperties()			{ return m_pSceneMaterialProperties; }	
		FORCEINLINE Buffer*				GetVertexBuffer()				{ return m_pSceneVertexBuffer; }		
		FORCEINLINE Buffer*				GetIndexBuffer()				{ return m_pSceneIndexBuffer; }		
		FORCEINLINE Buffer*				GetInstanceBufer()				{ return m_pSceneInstanceBuffer; }
		FORCEINLINE Buffer*				GetMeshIndexBuffer()			{ return m_pSceneMeshIndexBuffer; }	
																			
		

	private:
		const GraphicsDevice*						m_pGraphicsDevice;
		const IAudioDevice*							m_pAudioDevice;

		const char*									m_pName;

		CommandAllocator*							m_pCopyCommandAllocator					= nullptr;
		CommandAllocator*							m_pASBuildCommandAllocator				= nullptr;
		CommandList*								m_pCopyCommandList						= nullptr;
		CommandList*								m_pASBuildCommandList					= nullptr;

		std::map<uint32, uint32>					m_MaterialIndexToIndirectArgOffsetMap;
		std::vector<IndexedIndirectMeshArgument>	m_IndirectArgs;

		Buffer*									m_pLightsCopyBuffer						= nullptr;
		Buffer*									m_pPerFrameCopyBuffer					= nullptr;

		Buffer*									m_pSceneMaterialPropertiesCopyBuffer	= nullptr;
		Buffer*									m_pSceneVertexCopyBuffer				= nullptr;
		Buffer*									m_pSceneIndexCopyBuffer					= nullptr;
		Buffer*									m_pSceneInstanceCopyBuffer				= nullptr;
		Buffer*									m_pSceneMeshIndexCopyBuffer				= nullptr;

		Buffer*									m_pLightsBuffer							= nullptr;
		Buffer*									m_pPerFrameBuffer						= nullptr;

		std::vector<Texture*>						m_SceneAlbedoMaps;				
		std::vector<Texture*>						m_SceneNormalMaps;				
		std::vector<Texture*>						m_SceneAmbientOcclusionMaps;	
		std::vector<Texture*>						m_SceneMetallicMaps;			
		std::vector<Texture*>						m_SceneRoughnessMaps;			

		std::vector<TextureView*>					m_SceneAlbedoMapViews;
		std::vector<TextureView*>					m_SceneNormalMapViews;
		std::vector<TextureView*>					m_SceneAmbientOcclusionMapViews;
		std::vector<TextureView*>					m_SceneMetallicMapViews;
		std::vector<TextureView*>					m_SceneRoughnessMapViews;
		
		Buffer*									m_pSceneMaterialProperties		= nullptr;		//Indexed with result from IndirectMeshArgument::MaterialIndex, contains Scene Material Properties
		Buffer*									m_pSceneVertexBuffer			= nullptr;			//Indexed with result from Scene::m_pBaseVertexIndexBuffer + Scene::m_pSceneIndexBuffer and contains Scene Vertices
		Buffer*									m_pSceneIndexBuffer				= nullptr;			//Indexed with result from Scene::m_pMeshIndexBuffer + primitiveID * 3 + triangleCornerID and contains indices to Scene::m_pSceneVertexBuffer
		Buffer*									m_pSceneInstanceBuffer			= nullptr;			/*Indexed with InstanceID and contains per instance data, we can figure out the InstanceID during shading by using
																						IndirectMeshArgument::BaseInstanceIndex, IndirectMeshArgument::VertexCount and primitiveID <-- Relative to drawID*/

		Buffer*									m_pSceneMeshIndexBuffer			= nullptr;		/*Indexed with drawID when Shading and contains IndirectMeshArgument structs, primarily:
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
		AccelerationStructure*						m_pTLAS						= nullptr;
		std::vector<AccelerationStructure*>		m_BLASs;
	};
}
