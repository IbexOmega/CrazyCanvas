#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Resources/Mesh.h"

#include "Containers/TArray.h"

#include <map>

namespace LambdaEngine
{
	struct Mesh;
	struct Material;

	class IGraphicsDevice;
	class IAudioDevice;
	class ResourceManager;
	class IBuffer;
	class ITexture;
	
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
		uint32	BaseInstanceIndex	= 0;
	};

	struct SceneDesc
	{
		const char* pName					= "Scene";
	};

	class LAMBDA_API Scene
	{
		struct Instance
		{
			glm::mat3x4 Transform;
			uint32_t MeshMaterialIndex : 24;
			uint32_t Mask : 8;
			uint32_t SBTRecordOffset : 24;
			uint32_t Flags : 8;
			uint64_t AccelerationStructureHandle;
		};

		struct MappedMaterial
		{
			uint32 MaterialIndex = 0;  //Index to Scene::m_Materials
			std::vector<uint32> InstanceIndices;  //Indices to Scene::m_Instances
		};

		struct MappedMesh
		{
			std::map<GUID_Lambda, uint32> GUIDToMappedMaterials; //Mapping from GUID to Indices for MappedMesh::MappedMaterials
			std::vector<MappedMaterial>	MappedMaterials;
		};
		
	public:
		DECL_REMOVE_COPY(Scene);
		DECL_REMOVE_MOVE(Scene);

		Scene(const IGraphicsDevice* pGraphicsDevice, const IAudioDevice* pAudioDevice, const ResourceManager* pResourceManager);
		~Scene();

		bool Finalize(const SceneDesc& desc);

		uint32 AddStaticGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));
		uint32 AddDynamicGameObject(const GameObject& gameObject, const glm::mat4& transform = glm::mat4(1.0f));

		
		FORCEINLINE std::vector<ITexture*>&		GetAlbedoMaps()				{ return m_SceneAlbedoMaps; }			
		FORCEINLINE std::vector<ITexture*>&		GetNormalMaps()				{ return m_SceneNormalMaps; }			
		FORCEINLINE std::vector<ITexture*>&		GetAmbientOcclusionMaps()	{ return m_SceneAmbientOcclusionMaps; }
		FORCEINLINE std::vector<ITexture*>&		GetMetallicMaps()			{ return m_SceneMetallicMaps; }		
		FORCEINLINE std::vector<ITexture*>&		GetRoughnessMaps()			{ return m_SceneRoughnessMaps; }		
		FORCEINLINE IBuffer*					GetMaterialProperties()		{ return m_pSceneMaterialProperties; }	
		FORCEINLINE IBuffer*					GetVertexBuffer()			{ return m_pSceneVertexBuffer; }		
		FORCEINLINE IBuffer*					GetIndexBuffer()			{ return m_pSceneIndexBuffer; }		
		FORCEINLINE IBuffer*					GetInstanceBufer()			{ return m_pSceneInstanceBuffer; }
		FORCEINLINE IBuffer*					GetMeshIndexBuffer()		{ return m_pSceneMeshIndexBuffer; }	
																			
		

	private:
		const IGraphicsDevice*					m_pGraphicsDevice;
		const IAudioDevice*						m_pAudioDevice;
		const ResourceManager*					m_pResourceManager;

		const char*								m_pName;

		std::vector<ITexture*>					m_SceneAlbedoMaps;				//Indexed with result from IndirectMeshArgument::MaterialIndex, contains TextureMaps
		std::vector<ITexture*>					m_SceneNormalMaps;				//Indexed with result from IndirectMeshArgument::MaterialIndex, contains TextureMaps
		std::vector<ITexture*>					m_SceneAmbientOcclusionMaps;	//Indexed with result from IndirectMeshArgument::MaterialIndex, contains TextureMaps
		std::vector<ITexture*>					m_SceneMetallicMaps;			//Indexed with result from IndirectMeshArgument::MaterialIndex, contains TextureMaps
		std::vector<ITexture*>					m_SceneRoughnessMaps;			//Indexed with result from IndirectMeshArgument::MaterialIndex, contains TextureMaps
		
		IBuffer*								m_pSceneMaterialProperties;		//Indexed with result from IndirectMeshArgument::MaterialIndex, contains Scene Material Properties
		IBuffer*								m_pSceneVertexBuffer;			//Indexed with result from Scene::m_pBaseVertexIndexBuffer + Scene::m_pSceneIndexBuffer and contains Scene Vertices
		IBuffer*								m_pSceneIndexBuffer;			//Indexed with result from Scene::m_pMeshIndexBuffer + primitiveID * 3 + triangleCornerID and contains indices to Scene::m_pSceneVertexBuffer
		IBuffer*								m_pSceneInstanceBuffer;			/*Indexed with InstanceID and contains per instance data, we can figure out the InstanceID during shading by using
																					IndirectMeshArgument::BaseInstanceIndex, IndirectMeshArgument::VertexCount and primitiveID <-- Relative to drawID*/

		IBuffer*								m_pSceneMeshIndexBuffer;		/*Indexed with drawID when Shading and contains IndirectMeshArgument structs, primarily:
																					IndirectMeshArgument::FirstIndex		will be used as BaseIndex to m_pSceneIndexBuffer, 
																					IndirectMeshArgument::BaseVertexIndex	will be used as BaseIndex to m_pSceneVertexBuffer,
																					IndirectMeshArgument::MaterialIndex		will be used as MaterialIndex to MaterialBuffers,
																					IndirectMeshArgument::BaseInstanceIndex will be used as BaseIndex to m_pSceneInstanceBuffer*/


		std::map<GUID_Lambda, uint32>			m_GUIDToMappedMeshes;
		std::vector<MappedMesh>					m_MappedMeshes;
		std::vector<const Mesh*>				m_Meshes;
		std::vector<Vertex>						m_SceneVertexArray;
		std::vector<uint32>						m_SceneIndexArray;

		std::map<GUID_Lambda, uint32>			m_GUIDToMaterials;
		std::vector<const Material*>			m_Materials;

		std::vector<Instance>					m_Instances;
		std::vector<Instance>					m_SortedInstances;
	};
}
