#include "Resources/ResourceManager.h"

#include "Rendering/Core/API/TextureView.h"

#include "Log/Log.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/PipelineStateManager.h"

#include "Application/API/Events/EventQueue.h"

#include "Containers/TUniquePtr.h"

#include <assimp/postprocess.h>

#include <utility>
#include <unordered_set>

#define SAFEDELETE_ALL(map)     for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); }	map.clear()
#define SAFERELEASE_ALL(map)    for (auto it = map.begin(); it != map.end(); it++) { SAFERELEASE(it->second); }	map.clear()

#define REGISTER_TEXTURE_IN_MATERIAL(GUID, FALLBACK_GUID, pTexture, pTextureView) \
if (albedoMap != GUID_NONE) \
{ \
	pTexture		= s_Textures[GUID]; \
	pTextureView	= s_TextureViews[GUID]; \
	s_TextureMaterialRefs[albedoMap]++; \
} \
else \
{ \
	pTexture		= s_Textures[FALLBACK_GUID]; \
	pTextureView	= s_TextureViews[FALLBACK_GUID]; \
} \

#define REGISTER_COLOR_MAP_IN_MATERIAL(GUID, pTexture, pTextureView) REGISTER_TEXTURE_IN_MATERIAL(GUID, GUID_TEXTURE_DEFAULT_COLOR_MAP, pTexture, pTextureView)
#define REGISTER_NORMAL_MAP_IN_MATERIAL(GUID, pTexture, pTextureView) REGISTER_TEXTURE_IN_MATERIAL(GUID, GUID_TEXTURE_DEFAULT_NORMAL_MAP, pTexture, pTextureView)

namespace LambdaEngine
{
	GUID_Lambda ResourceManager::s_NextFreeGUID = SMALLEST_UNRESERVED_GUID;

	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_MeshGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_MaterialGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_AnimationGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_TextureGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_ShaderGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_SoundEffect3DGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_SoundEffect2DGUIDsToNames;
	std::unordered_map<GUID_Lambda, String>				ResourceManager::s_MusicGUIDsToNames;

	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_MeshNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_MaterialNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_AnimationNamesToGUIDs;
	std::unordered_map<String, TArray<GUID_Lambda>>		ResourceManager::s_FileNamesToAnimationGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_TextureNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_ShaderNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_SoundEffect3DNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_SoundEffect2DNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>				ResourceManager::s_MusicNamesToGUIDs;

	std::unordered_map<GUID_Lambda, Mesh*>				ResourceManager::s_Meshes;
	std::unordered_map<GUID_Lambda, Material*>			ResourceManager::s_Materials;
	std::unordered_map<GUID_Lambda, Animation*>			ResourceManager::s_Animations;
	std::unordered_map<GUID_Lambda, Texture*>			ResourceManager::s_Textures;
	std::unordered_map<GUID_Lambda, TextureView*>		ResourceManager::s_TextureViews;
	std::unordered_map<GUID_Lambda, Shader*>			ResourceManager::s_Shaders;
	std::unordered_map<GUID_Lambda, ISoundEffect3D*>	ResourceManager::s_SoundEffects3D;
	std::unordered_map<GUID_Lambda, ISoundEffect2D*>	ResourceManager::s_SoundEffects2D;
	std::unordered_map<GUID_Lambda, IMusic*>			ResourceManager::s_Music;

	std::unordered_map<GUID_Lambda, uint32>								ResourceManager::s_TextureMaterialRefs;
	std::unordered_map<GUID_Lambda, ResourceManager::MaterialLoadDesc>	ResourceManager::s_MaterialLoadConfigurations;
	std::unordered_map<GUID_Lambda, ResourceManager::ShaderLoadDesc>	ResourceManager::s_ShaderLoadConfigurations;

	CommandAllocator* ResourceManager::s_pMaterialComputeCommandAllocator	= nullptr;
	CommandAllocator* ResourceManager::s_pMaterialGraphicsCommandAllocator	= nullptr;

	CommandList* ResourceManager::s_pMaterialComputeCommandList		= nullptr;
	CommandList* ResourceManager::s_pMaterialGraphicsCommandList	= nullptr;

	Fence* ResourceManager::s_pMaterialFence = nullptr;

	DescriptorHeap* ResourceManager::s_pMaterialDescriptorHeap	= nullptr;
	DescriptorSet* ResourceManager::s_pMaterialDescriptorSet	= nullptr;

	PipelineLayout* ResourceManager::s_pMaterialPipelineLayout	= nullptr;

	PipelineState* ResourceManager::s_pAllChannelsSeperateMaterialPipelineState	= nullptr;
	PipelineState* ResourceManager::s_pAOSeperateMetRoughCombinedMaterialPipelineState	= nullptr;

	GUID_Lambda ResourceManager::s_AllChannelsSeperateMaterialShaderGUID = GUID_NONE;
	GUID_Lambda ResourceManager::s_AOSeperateMetRoughCombinedMaterialShaderGUID = GUID_NONE;

	TSet<GUID_Lambda> ResourceManager::s_UnloadedGUIDs;

	bool ResourceManager::Init()
	{
		InitMaterialCreation();
		InitDefaultResources();

		EventQueue::RegisterEventHandler<ShaderRecompileEvent>(&OnShaderRecompileEvent);

		return true;
	}

	bool ResourceManager::Release()
	{
		EventQueue::UnregisterEventHandler<ShaderRecompileEvent>(&OnShaderRecompileEvent);

		ReleaseMaterialCreation();

		SAFEDELETE_ALL(s_Meshes);
		SAFEDELETE_ALL(s_Materials);
		SAFEDELETE_ALL(s_Animations);
		SAFEDELETE_ALL(s_SoundEffects3D);
		SAFEDELETE_ALL(s_SoundEffects2D);
		SAFEDELETE_ALL(s_Music);

		// TODO: Change to TSharedRef would prevent for the need of these
		SAFERELEASE_ALL(s_Textures);
		SAFERELEASE_ALL(s_TextureViews);
		SAFERELEASE_ALL(s_Shaders);

		return true;
	}

	bool ResourceManager::LoadSceneFromFile(
		const SceneLoadDesc* pSceneLoadDesc,
		TArray<MeshComponent>& meshComponents,
		TArray<LoadedDirectionalLight>& directionalLights,
		TArray<LoadedPointLight>& pointLights,
		TArray<LevelObjectOnLoad>& levelObjects,
		const String& directory)
	{
		VALIDATE(pSceneLoadDesc != nullptr);

		TArray<MeshComponent>	sceneLocalMeshComponents;
		TArray<Mesh*>			meshes;
		TArray<Animation*>		animations;
		TArray<LoadedMaterial*>	materials;
		TArray<LoadedTexture*>	textures;

		if (!ResourceLoader::LoadSceneFromFile(
			directory + pSceneLoadDesc->Filename,
			pSceneLoadDesc->LevelObjectDescriptions,
			sceneLocalMeshComponents,
			directionalLights,
			pointLights,
			levelObjects,
			meshes, 
			animations, 
			materials, 
			textures))
		{
			return false;
		}

		TArray<MaterialLoadDesc> materialLoadConfigurations;
		materialLoadConfigurations.Resize(materials.GetSize());

		TArray<TextureView*> textureViewsToDelete;

		//Spinlock yes no?

		meshComponents = sceneLocalMeshComponents;
		for (uint32 i = 0; i < textures.GetSize(); i++)
		{
			LoadedTexture* pLoadedTexture = textures[i];

			RegisterLoadedMaterialTexture(
				pLoadedTexture,
				materials,
				materialLoadConfigurations,
				textureViewsToDelete);
		}

		for (uint32 i = 0; i < meshes.GetSize(); i++)
		{
			GUID_Lambda guid = RegisterMesh("Scene Mesh " + std::to_string(i), meshes[i]);

			//Loop through mesh component and set the real mesh GUID
			for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
			{
				if (sceneLocalMeshComponents[g].MeshGUID == i)
				{
					meshComponents[g].MeshGUID = guid;
				}
			}

			//Loop through special objects and set their mesh component material GUIDs
			for (uint32 s = 0; s < levelObjects.GetSize(); s++)
			{
				LevelObjectOnLoad* pLevelObject = &levelObjects[s];

				for (MeshComponent& meshComponent : pLevelObject->MeshComponents)
				{
					if (meshComponent.MeshGUID == i)
						meshComponent.MeshGUID = guid;
				}
			}
		}

		for (uint32 i = 0; i < materials.GetSize(); i++)
		{
			LoadedMaterial* pLoadedMaterial = materials[i];
			MaterialLoadDesc& materialLoadConfig = materialLoadConfigurations[i];

			GUID_Lambda guid = RegisterLoadedMaterial(
				"Scene Material " + std::to_string(i), 
				pLoadedMaterial, 
				materialLoadConfig);

			//Loop through mesh component and set the real material GUID
			for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
			{
				if (sceneLocalMeshComponents[g].MaterialGUID == i)
				{
					meshComponents[g].MaterialGUID = guid;
				}
			}

			//Loop through level objects and set their mesh component material GUIDs
			for (uint32 s = 0; s < levelObjects.GetSize(); s++)
			{
				LevelObjectOnLoad* pLevelObject = &levelObjects[s];

				for (MeshComponent& meshComponent : pLevelObject->MeshComponents)
				{
					if (meshComponent.MaterialGUID == i)
						meshComponent.MaterialGUID = guid;
				}
			}

			SAFEDELETE(pLoadedMaterial);
		}

		for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
		{
			if (sceneLocalMeshComponents[g].MeshGUID >= meshes.GetSize())
			{
				LOG_ERROR("[ResourceManager]: GameObject %u in Scene %s has no Mesh", g, pSceneLoadDesc->Filename.c_str());
			}

			if (sceneLocalMeshComponents[g].MaterialGUID >= materials.GetSize())
			{
				meshComponents[g].MaterialGUID = GUID_MATERIAL_DEFAULT;
				LOG_WARNING("[ResourceManager]: GameObject %u in Scene %s has no Material, default Material assigned", g, pSceneLoadDesc->Filename.c_str());
			}
		}

		//Delete AO, Metallic, Roughness & Metallic/Roughness Textures
		for (uint32 i = 0; i < textures.GetSize(); i++)
		{
			LoadedTexture* pLoadedTexture = textures[i];

			if ((pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ALBEDO) == 0 &&
				(pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL) == 0)
			{
				SAFERELEASE(pLoadedTexture->pTexture);
			}

			SAFEDELETE(pLoadedTexture);
		}
		textures.Clear();

		for (uint32 i = 0; i < textureViewsToDelete.GetSize(); i++)
		{
			SAFERELEASE(textureViewsToDelete[i]);
		}
		textureViewsToDelete.Clear();

		return true;
	}

	void ResourceManager::LoadMeshFromFile(const String& filename, GUID_Lambda& meshGUID)
	{
		if (auto loadedMeshGUID = s_MeshNamesToGUIDs.find(filename); loadedMeshGUID != s_MeshNamesToGUIDs.end())
		{
			meshGUID = loadedMeshGUID->second;
		}

		int32 assimpFlags =
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace |
			aiProcess_FindInstances |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_Triangulate |
			aiProcess_GenUVCoords |
			aiProcess_FindDegenerates |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FindInvalidData;

		Mesh* pMesh = ResourceLoader::LoadMeshFromFile(MESH_DIR + filename, nullptr, nullptr, nullptr, assimpFlags);

		//Spinlock
		{
			meshGUID						= s_NextFreeGUID++;
			s_Meshes[meshGUID]				= pMesh;
			s_MeshGUIDsToNames[meshGUID]	= filename;
			s_MeshNamesToGUIDs[filename]	= meshGUID;
		}
	}

	void ResourceManager::LoadMeshFromFile(const String& filename, GUID_Lambda& meshGUID, TArray<GUID_Lambda>& animations)
	{
		if (auto loadedMeshGUID = s_MeshNamesToGUIDs.find(filename); loadedMeshGUID != s_MeshNamesToGUIDs.end())
		{
			meshGUID = loadedMeshGUID->second;

			if (auto loadedAnimations = s_FileNamesToAnimationGUIDs.find(filename); loadedAnimations != s_FileNamesToAnimationGUIDs.end())
			{
				animations = loadedAnimations->second;
			}
		}

		int32 assimpFlags =
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace |
			aiProcess_FindInstances |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_Triangulate |
			aiProcess_GenUVCoords |
			aiProcess_FindDegenerates |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FindInvalidData;

		TArray<Animation*> rawAnimations;
		Mesh* pMesh = ResourceLoader::LoadMeshFromFile(MESH_DIR + filename, nullptr, nullptr, &rawAnimations, assimpFlags);

		//Spinlock
		{
			meshGUID						= s_NextFreeGUID++;
			s_Meshes[meshGUID]				= pMesh;
			s_MeshGUIDsToNames[meshGUID]	= filename;
			s_MeshNamesToGUIDs[filename]	= meshGUID;
		}

		// Register animations
		animations.Clear();
		animations.Reserve(rawAnimations.GetSize());
		for (Animation* pAnimation : rawAnimations)
		{
			VALIDATE(pAnimation);

			GUID_Lambda animationsGuid = RegisterAnimation(pAnimation->Name, pAnimation);
			animations.EmplaceBack(animationsGuid);
		}

		s_FileNamesToAnimationGUIDs.insert(std::make_pair(filename, animations));
	}

	void ResourceManager::LoadMeshAndMaterialFromFile(const String& filename, GUID_Lambda& meshGUID, GUID_Lambda& materialGUID)
	{
		if (auto loadedMeshGUID = s_MeshNamesToGUIDs.find(filename); loadedMeshGUID != s_MeshNamesToGUIDs.end())
		{
			meshGUID = loadedMeshGUID->second;

			auto loadedMaterialGUID = s_MaterialNamesToGUIDs.find(filename);
			materialGUID = loadedMaterialGUID != s_MaterialNamesToGUIDs.end() ? loadedMaterialGUID->second : GUID_MATERIAL_DEFAULT;

			return;
		}

		TArray<LoadedMaterial*> materials;
		TArray<LoadedTexture*> textures;
		TArray<TextureView*> textureViewsToDelete;

		int32 assimpFlags =
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace |
			aiProcess_FindInstances |
			aiProcess_FixInfacingNormals |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_Triangulate |
			aiProcess_GenUVCoords |
			aiProcess_FindDegenerates |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FindInvalidData;

			// Prevent crashes in assimp when using this flag
			//String path = ConvertSlashes(filepath);
			//if (path.find(".obj") == String::npos && path.find(".glb") == String::npos)
			//{
			//	assimpFlags |= aiProcess_PopulateArmatureData;
			//}

		Mesh* pMesh = ResourceLoader::LoadMeshFromFile(MESH_DIR + filename, &materials, &textures, nullptr, assimpFlags);

		//Spinlock
		{
			meshGUID						= s_NextFreeGUID++;
			s_Meshes[meshGUID]				= pMesh;
			s_MeshGUIDsToNames[meshGUID]	= filename;
			s_MeshNamesToGUIDs[filename]	= meshGUID;

			if (!materials.IsEmpty())
			{
				if (materials.GetSize() > 1)
				{
					LOG_WARNING("[ResourceManager]: Mesh %s loaded with more than one material, the other materials will be ignored");
				}

				LoadedMaterial* pMaterial = materials[0];
				MaterialLoadDesc materialLoadDesc;

				for (LoadedTexture* pLoadedTexture : textures)
				{
					RegisterLoadedMaterialTexture(
						pLoadedTexture,
						pMaterial,
						materialLoadDesc,
						textureViewsToDelete);
				}

				materialGUID = RegisterLoadedMaterial(
					filename,
					pMaterial,
					materialLoadDesc);
			}
			else
			{
				materialGUID = GUID_MATERIAL_DEFAULT;
			}
		}

		//Delete Material Wrappers
		for (LoadedMaterial* pLoadedMaterial : materials)
		{
			SAFEDELETE(pLoadedMaterial);
		}

		//Delete AO, Metallic, Roughness & Metallic/Roughness Textures
		for (LoadedTexture* pLoadedTexture : textures)
		{
			if ((pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ALBEDO) == 0 &&
				(pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL) == 0)
			{
				SAFERELEASE(pLoadedTexture->pTexture);
			}

			SAFEDELETE(pLoadedTexture);
		}

		//Delete Temp Texture Views used to Generate Combined Materials
		for (TextureView* pTextureViewToDelete : textureViewsToDelete)
		{
			SAFERELEASE(pTextureViewToDelete);
		}
	}

	void ResourceManager::LoadMeshAndMaterialFromFile(const String& filename, GUID_Lambda& meshGUID, GUID_Lambda& materialGUID, TArray<GUID_Lambda>& animations)
	{
		if (auto loadedMeshGUID = s_MeshNamesToGUIDs.find(filename); loadedMeshGUID != s_MeshNamesToGUIDs.end())
		{
			meshGUID = loadedMeshGUID->second;

			if (auto loadedAnimations = s_FileNamesToAnimationGUIDs.find(filename); loadedAnimations != s_FileNamesToAnimationGUIDs.end())
			{
				animations = loadedAnimations->second;
			}

			auto loadedMaterialGUID = s_MaterialNamesToGUIDs.find(filename);
			materialGUID = loadedMaterialGUID != s_MaterialNamesToGUIDs.end() ? loadedMaterialGUID->second : GUID_MATERIAL_DEFAULT;
		}

		int32 assimpFlags =
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace |
			aiProcess_FindInstances |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_Triangulate |
			aiProcess_GenUVCoords |
			aiProcess_FindDegenerates |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FindInvalidData;

		TArray<Animation*> rawAnimations;
		TArray<LoadedMaterial*> materials;
		TArray<LoadedTexture*> textures;
		TArray<TextureView*> textureViewsToDelete;
		Mesh* pMesh = ResourceLoader::LoadMeshFromFile(MESH_DIR + filename, &materials, &textures, &rawAnimations, assimpFlags);

		//Spinlock
		{
			meshGUID						= s_NextFreeGUID++;
			s_Meshes[meshGUID]				= pMesh;
			s_MeshGUIDsToNames[meshGUID]	= filename;
			s_MeshNamesToGUIDs[filename]	= meshGUID;

			if (!materials.IsEmpty())
			{
				if (materials.GetSize() > 1)
				{
					LOG_WARNING("[ResourceManager]: Mesh %s loaded with more than one material, the other materials will be ignored");
				}

				LoadedMaterial* pMaterial = materials[0];
				MaterialLoadDesc materialLoadDesc;

				for (LoadedTexture* pLoadedTexture : textures)
				{
					RegisterLoadedMaterialTexture(
						pLoadedTexture,
						pMaterial,
						materialLoadDesc,
						textureViewsToDelete);
				}

				materialGUID = RegisterLoadedMaterial(
					filename,
					pMaterial,
					materialLoadDesc);
			}
			else
			{
				materialGUID = GUID_MATERIAL_DEFAULT;
			}
		}

		// Register animations
		animations.Clear();
		animations.Reserve(rawAnimations.GetSize());
		for (Animation* pAnimation : rawAnimations)
		{
			VALIDATE(pAnimation);

			GUID_Lambda animationsGuid = RegisterAnimation(pAnimation->Name, pAnimation);
			animations.EmplaceBack(animationsGuid);
		}

		s_FileNamesToAnimationGUIDs.insert(std::make_pair(filename, animations));

		//Delete Material Wrappers
		for (LoadedMaterial* pLoadedMaterial : materials)
		{
			SAFEDELETE(pLoadedMaterial);
		}

		//Delete AO, Metallic, Roughness & Metallic/Roughness Textures
		for (LoadedTexture* pLoadedTexture : textures)
		{
			if ((pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ALBEDO) == 0 &&
				(pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL) == 0)
			{
				SAFERELEASE(pLoadedTexture->pTexture);
			}

			SAFEDELETE(pLoadedTexture);
		}

		//Delete Temp Texture Views used to Generate Combined Materials
		for (TextureView* pTextureViewToDelete : textureViewsToDelete)
		{
			SAFERELEASE(pTextureViewToDelete);
		}
	}

	TArray<GUID_Lambda> ResourceManager::LoadAnimationsFromFile(const String& filename)
	{
		TArray<GUID_Lambda> animations;

		auto loadedAnimations = s_FileNamesToAnimationGUIDs.find(filename);
		if (loadedAnimations != s_FileNamesToAnimationGUIDs.end())
		{
			animations = loadedAnimations->second;
			return animations;
		}

		// Load animations
		TArray<Animation*> rawAnimations = ResourceLoader::LoadAnimationsFromFile(ANIMATIONS_DIR + filename);

		// Register animations
		animations.Reserve(rawAnimations.GetSize());
		for (Animation* pAnimation : rawAnimations)
		{
			VALIDATE(pAnimation);

			GUID_Lambda animationsGuid = RegisterAnimation(pAnimation->Name, pAnimation);
			animations.EmplaceBack(animationsGuid);
		}

		s_FileNamesToAnimationGUIDs.insert(std::make_pair(filename, animations));
		return animations;
	}

	GUID_Lambda ResourceManager::LoadMeshFromMemory(const String& name, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		auto loadedMeshGUID = s_MeshNamesToGUIDs.find(name);
		if (loadedMeshGUID != s_MeshNamesToGUIDs.end())
		{
			return loadedMeshGUID->second;
		}

		GUID_Lambda	guid = GUID_NONE;
		Mesh**		ppMappedMesh = nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedMesh				= &s_Meshes[guid]; //Creates new entry if not existing
			s_MeshGUIDsToNames[guid]	= name;
			s_MeshNamesToGUIDs[name]	= guid;
		}

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromMemory(pVertices, numVertices, pIndices, numIndices);
		return guid;
	}

	GUID_Lambda ResourceManager::LoadMaterialFromMemory(
		const String& name, 
		GUID_Lambda albedoMap, 
		GUID_Lambda normalMap, 
		GUID_Lambda ambientOcclusionMap, 
		GUID_Lambda metallicMap, 
		GUID_Lambda roughnessMap, 
		const MaterialProperties& properties)
	{
		auto loadedMaterialGUID = s_MaterialNamesToGUIDs.find(name);
		if (loadedMaterialGUID != s_MaterialNamesToGUIDs.end())
		{
			return loadedMaterialGUID->second;
		}

		GUID_Lambda guid				= GUID_NONE;
		Material**	ppMappedMaterial	 = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedMaterial				= &s_Materials[guid]; //Creates new entry if not existing
			s_MaterialGUIDsToNames[guid]	= name;
			s_MaterialNamesToGUIDs[name]	= guid;
		}

		(*ppMappedMaterial) = DBG_NEW Material();

		Texture* pAlbedoMap;
		Texture* pNormalMap;
		Texture* pAmbientOcclusionMap;
		Texture* pMetallicMap;
		Texture* pRoughnessMap;

		TextureView* pAlbedoMapView;
		TextureView* pNormalMapView;
		TextureView* pAmbientOcclusionMapView;
		TextureView* pMetallicMapView;
		TextureView* pRoughnessMapView;

		REGISTER_COLOR_MAP_IN_MATERIAL(albedoMap, pAlbedoMap, pAlbedoMapView);
		REGISTER_NORMAL_MAP_IN_MATERIAL(normalMap, pNormalMap, pNormalMapView);
		REGISTER_COLOR_MAP_IN_MATERIAL(ambientOcclusionMap, pAmbientOcclusionMap, pAmbientOcclusionMapView);
		REGISTER_COLOR_MAP_IN_MATERIAL(metallicMap, pMetallicMap, pMetallicMapView);
		REGISTER_COLOR_MAP_IN_MATERIAL(roughnessMap, pRoughnessMap, pRoughnessMapView);

		(*ppMappedMaterial)->Properties = properties;

		(*ppMappedMaterial)->pAlbedoMap = pAlbedoMap;
		(*ppMappedMaterial)->pNormalMap = pNormalMap;

		(*ppMappedMaterial)->pAlbedoMapView = pAlbedoMapView;
		(*ppMappedMaterial)->pNormalMapView = pNormalMapView;

		GUID_Lambda aoMetallicRoughnessGUID = CombineMaterialTextures(
			name + " Combined Material Texture",
			*ppMappedMaterial,
			pAmbientOcclusionMap,
			pMetallicMap,
			pRoughnessMap,
			nullptr,
			pAmbientOcclusionMapView,
			pMetallicMapView,
			pRoughnessMapView,
			nullptr);

		MaterialLoadDesc materialLoadDesc = {};
		materialLoadDesc.AlbedoMapGUID				= albedoMap;
		materialLoadDesc.NormalMapGUID				= normalMap;
		materialLoadDesc.AOMapGUID					= ambientOcclusionMap;
		materialLoadDesc.MetallicMapGUID			= metallicMap;
		materialLoadDesc.RoughnessMapGUID			= roughnessMap;
		materialLoadDesc.AOMetallicRoughnessMapGUID	= aoMetallicRoughnessGUID;

		s_MaterialLoadConfigurations[guid] = materialLoadDesc;

		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips, bool linearFilteringMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
		{
			return loadedTextureGUID->second;
		}

		GUID_Lambda		guid = GUID_NONE;
		Texture**		ppMappedTexture = nullptr;
		TextureView**	ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedTexture				= &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView			= &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureGUIDsToNames[guid]	= name;
			s_TextureNamesToGUIDs[name]	= guid;
		}

		Texture* pTexture = ResourceLoader::LoadTextureArrayFromFile(name, TEXTURE_DIR, pFilenames, count, format, generateMips, linearFilteringMips);

		(*ppMappedTexture) = pTexture;

		TextureDesc textureDesc = pTexture->GetDesc();

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= name + " Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= format;
		textureViewDesc.Type			= textureDesc.ArrayCount > 1 ? ETextureViewType::TEXTURE_VIEW_TYPE_2D_ARRAY : ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= textureDesc.Miplevels;
		textureViewDesc.ArrayCount		= textureDesc.ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
		return guid;
	}

	GUID_Lambda ResourceManager::LoadCubeTexturesArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips, bool linearFilteringMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
		{
			return loadedTextureGUID->second;
		}

		uint32 textureCount = count * 6U;

		GUID_Lambda		guid				= GUID_NONE;
		Texture**		ppMappedTexture		= nullptr;
		TextureView**	ppMappedTextureView	= nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedTexture				= &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView			= &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureGUIDsToNames[guid] = name;
			s_TextureNamesToGUIDs[name]	= guid;
		}

		Texture* pTexture = ResourceLoader::LoadCubeTexturesArrayFromFile(name, TEXTURE_DIR, pFilenames, textureCount, format, generateMips, linearFilteringMips);

		(*ppMappedTexture) = pTexture;

		TextureDesc textureDesc = pTexture->GetDesc();

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= name + " Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= format;
		textureViewDesc.Type			= count > 1 ? ETextureViewType::TEXTURE_VIEW_TYPE_CUBE_ARRAY : ETextureViewType::TEXTURE_VIEW_TYPE_CUBE;
		textureViewDesc.MiplevelCount	= textureDesc.Miplevels;
		textureViewDesc.ArrayCount		= textureDesc.ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureFromFile(const String& filename, EFormat format, bool generateMips, bool linearFiltering)
	{
		return LoadTextureArrayFromFile(filename, &filename, 1, format, generateMips, linearFiltering);
	}

	GUID_Lambda ResourceManager::LoadTextureFromMemory(const String& name, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips, bool linearFilteringMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
		{
			return loadedTextureGUID->second;
		}

		GUID_Lambda		guid				= GUID_NONE;
		Texture**		ppMappedTexture		=	nullptr;
		TextureView**	ppMappedTextureView	= nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedTexture				= &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView			= &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureGUIDsToNames[guid] = name;
			s_TextureNamesToGUIDs[name]	= guid;
		}

		Texture* pTexture = ResourceLoader::LoadTextureArrayFromMemory(name, &pData, 1, width, height, format, usageFlags, generateMips, linearFilteringMips);

		(*ppMappedTexture) = pTexture;

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= name + " Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadShaderFromFile(const String& filename, FShaderStageFlag stage, EShaderLang lang, const char* pEntryPoint)
	{
		auto loadedShaderGUID = s_ShaderNamesToGUIDs.find(filename);
		if (loadedShaderGUID != s_ShaderNamesToGUIDs.end())
		{
			return loadedShaderGUID->second;
		}

		GUID_Lambda guid = GUID_NONE;
		Shader** ppMappedShader = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedShader					= &s_Shaders[guid]; //Creates new entry if not existing
			s_ShaderGUIDsToNames[guid]		= filename;
			s_ShaderNamesToGUIDs[filename]	= guid;
		}

		String filepath = SHADER_DIR + filename;

		ShaderLoadDesc loadDesc = {};
		loadDesc.Filepath		= filepath;
		loadDesc.Stage			= stage;
		loadDesc.Lang			= lang;
		loadDesc.pEntryPoint	= pEntryPoint;

		s_ShaderLoadConfigurations[guid] = loadDesc;

		(*ppMappedShader) = ResourceLoader::LoadShaderFromFile(filepath, stage, lang, pEntryPoint);

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterShader(const String& name, Shader* pShader)
	{
		auto loadedShaderGUID = s_ShaderNamesToGUIDs.find(name);
		if (loadedShaderGUID != s_ShaderNamesToGUIDs.end())
			return loadedShaderGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Shader** ppMappedShader = nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedShader				= &s_Shaders[guid]; //Creates new entry if not existing
			s_ShaderGUIDsToNames[guid]	= name;
			s_ShaderNamesToGUIDs[name]	= guid;
		}

		(*ppMappedShader) = pShader;

		return guid;
	}

	GUID_Lambda ResourceManager::LoadSoundEffect3DFromFile(const String& filename)
	{
		auto loadedSoundEffectGUID = s_SoundEffect3DNamesToGUIDs.find(filename);
		if (loadedSoundEffectGUID != s_SoundEffect3DNamesToGUIDs.end())
		{
			return loadedSoundEffectGUID->second;
		}

		GUID_Lambda			guid = GUID_NONE;
		ISoundEffect3D**	ppMappedSoundEffect = nullptr;

		//Spinlock
		{
			guid									= s_NextFreeGUID++;
			ppMappedSoundEffect						= &s_SoundEffects3D[guid]; //Creates new entry if not existing
			s_SoundEffect3DGUIDsToNames[guid]		= filename;
			s_SoundEffect3DNamesToGUIDs[filename]	= guid;
		}

		(*ppMappedSoundEffect) = ResourceLoader::LoadSoundEffect3DFromFile(SOUND_DIR + filename);
		return guid;
	}

	GUID_Lambda ResourceManager::LoadSoundEffect2DFromFile(const String& filename)
	{
		auto loadedSoundEffectGUID = s_SoundEffect2DNamesToGUIDs.find(filename);
		if (loadedSoundEffectGUID != s_SoundEffect2DNamesToGUIDs.end())
		{
			return loadedSoundEffectGUID->second;
		}

		GUID_Lambda			guid = GUID_NONE;
		ISoundEffect2D**	ppMappedSoundEffect = nullptr;

		//Spinlock
		{
			guid									= s_NextFreeGUID++;
			ppMappedSoundEffect						= &s_SoundEffects2D[guid]; //Creates new entry if not existing
			s_SoundEffect2DGUIDsToNames[guid]		= filename;
			s_SoundEffect2DNamesToGUIDs[filename]	= guid;
		}

		(*ppMappedSoundEffect) = ResourceLoader::LoadSoundEffect2DFromFile(SOUND_DIR + filename);
		return guid;
	}

	GUID_Lambda ResourceManager::LoadMusicFromFile(const String& filename, float32 defaultVolume, float32 defaultPitch)
	{
		auto loadedMusicGUID = s_MusicNamesToGUIDs.find(filename);
		if (loadedMusicGUID != s_MusicNamesToGUIDs.end())
		{
			return loadedMusicGUID->second;
		}

		GUID_Lambda	guid = GUID_NONE;
		IMusic**	ppMappedMusic = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedMusic					= &s_Music[guid]; //Creates new entry if not existing
			s_MusicGUIDsToNames[guid]		= filename;
			s_MusicNamesToGUIDs[filename]	= guid;
		}

		(*ppMappedMusic) = ResourceLoader::LoadMusicFromFile(SOUND_DIR + filename, defaultVolume, defaultPitch);
		return guid;
	}

	GUID_Lambda ResourceManager::CombineMaterialTextures(
		const String& combinedTextureName,
		Material* pMaterial,
		Texture* pAOMap,
		Texture* pMetallicMap,
		Texture* pRoughnessMap,
		Texture* pMetallicRoughnessMap,
		TextureView* pAOMapView,
		TextureView* pMetallicMapView,
		TextureView* pRoughnessMapView,
		TextureView* pMetallicRoughnessMapView)
	{
		uint32 largestWidth;
		uint32 largestHeight;

		// Find largest texture size
		if (pMetallicRoughnessMap == nullptr)
		{
			largestWidth	= std::max(pMetallicMap->GetDesc().Width, std::max(pRoughnessMap->GetDesc().Width, pAOMap->GetDesc().Width));
			largestHeight	= std::max(pMetallicMap->GetDesc().Height, std::max(pRoughnessMap->GetDesc().Height, pAOMap->GetDesc().Height));
		}
		else
		{
			largestWidth	= std::max(pMetallicRoughnessMap->GetDesc().Width, pAOMap->GetDesc().Width);
			largestHeight	= std::max(pMetallicRoughnessMap->GetDesc().Height, pAOMap->GetDesc().Height);
		}

		uint32_t miplevels = 1u;
		miplevels = uint32(glm::floor(glm::log2((float)glm::max(largestWidth, largestHeight)))) + 1u;

		Texture* pCombinedMaterialTexture;
		TextureView* pCombinedMaterialTextureView;

		//Create new Combined Material Texture & Texture View
		{
			TextureDesc textureDesc = { };
			textureDesc.DebugName		= combinedTextureName;
			textureDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
			textureDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
			textureDesc.Type			= ETextureType::TEXTURE_TYPE_2D;
			textureDesc.Flags			= FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_UNORDERED_ACCESS | FTextureFlag::TEXTURE_FLAG_COPY_SRC | FTextureFlag::TEXTURE_FLAG_COPY_DST;
			textureDesc.Width			= largestWidth;
			textureDesc.Height			= largestHeight;
			textureDesc.Depth			= 1;
			textureDesc.ArrayCount		= 1;
			textureDesc.Miplevels		= miplevels;
			textureDesc.SampleCount		= 1;

			pCombinedMaterialTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);

			if (pCombinedMaterialTexture == nullptr)
			{
				LOG_ERROR("[ResourceLoader]: Failed to create texture for \"Combined Material Texture\"");
				return GUID_NONE;
			}

			TextureViewDesc textureViewDesc;
			textureViewDesc.DebugName		= combinedTextureName + " Texture View";
			textureViewDesc.pTexture		= pCombinedMaterialTexture;
			textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS;
			textureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= miplevels;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;

			pCombinedMaterialTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

			if (pCombinedMaterialTextureView == nullptr)
			{
				LOG_ERROR("[ResourceLoader]: Failed to create texture view for \"Combined Material Texture View\"");
				return GUID_NONE;
			}
		}

		//Update Descriptor Set
		{
			s_pMaterialDescriptorSet->WriteTextureDescriptors(
				&pCombinedMaterialTextureView,
				nullptr,
				ETextureState::TEXTURE_STATE_GENERAL,
				0,
				1,
				EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE,
				true);

			s_pMaterialDescriptorSet->WriteTextureDescriptors(
				&pAOMapView,
				nullptr,
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
				1,
				1,
				EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
				true);

			if (pMetallicRoughnessMap == nullptr)
			{
				//Metallic and Roughness Channels are seperate textures

				s_pMaterialDescriptorSet->WriteTextureDescriptors(
					&pMetallicMapView,
					nullptr,
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					2,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					true);

				s_pMaterialDescriptorSet->WriteTextureDescriptors(
					&pRoughnessMapView,
					nullptr,
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					3,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					true);
			}
			else
			{
				s_pMaterialDescriptorSet->WriteTextureDescriptors(
					&pMetallicRoughnessMapView,
					nullptr,
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					2,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					true);
			}
		}

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = { };
		transitionToCopyDstBarrier.pTexture					= pCombinedMaterialTexture;
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_GENERAL;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= pCombinedMaterialTexture->GetDesc().Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= pCombinedMaterialTexture->GetDesc().Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= pCombinedMaterialTexture->GetDesc().ArrayCount;

		static uint64 signalValue = 0;

		//Execute Compute Pass
		{
			s_pMaterialComputeCommandAllocator->Reset();
			s_pMaterialComputeCommandList->Begin(nullptr);

			s_pMaterialComputeCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);

			s_pMaterialComputeCommandList->BindDescriptorSetCompute(s_pMaterialDescriptorSet, s_pMaterialPipelineLayout, 0);

			if (pMetallicRoughnessMap == nullptr)
			{
				s_pMaterialComputeCommandList->BindComputePipeline(s_pAllChannelsSeperateMaterialPipelineState);
			}
			else
			{
				s_pMaterialComputeCommandList->BindComputePipeline(s_pAOSeperateMetRoughCombinedMaterialPipelineState);
			}

			// Dispatch
			largestWidth = std::max<uint32>(largestWidth / 8, 1);
			largestHeight = std::max<uint32>(largestHeight / 8, 1);

			s_pMaterialComputeCommandList->Dispatch(largestWidth, largestHeight, 1);

			s_pMaterialComputeCommandList->QueueTransferBarrier(
				pCombinedMaterialTexture,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE,
				ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS,
				ETextureState::TEXTURE_STATE_GENERAL,
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);

			s_pMaterialComputeCommandList->End();

			signalValue++;
			RenderAPI::GetComputeQueue()->ExecuteCommandLists(&s_pMaterialComputeCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, s_pMaterialFence, signalValue);
		}

		//Execute Mipmap Pass
		s_pMaterialGraphicsCommandAllocator->Reset();
		s_pMaterialGraphicsCommandList->Begin(nullptr);

		if (miplevels > 1)
		{
			s_pMaterialGraphicsCommandList->QueueTransferBarrier(
				pCombinedMaterialTexture,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE,
				ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS,
				ETextureState::TEXTURE_STATE_GENERAL,
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);

			s_pMaterialGraphicsCommandList->GenerateMiplevels(pCombinedMaterialTexture, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, true);
		}
		else
		{
			s_pMaterialGraphicsCommandList->QueueTransferBarrier(
				pCombinedMaterialTexture,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP,
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE,
				FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ,
				ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE,
				ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS,
				ETextureState::TEXTURE_STATE_GENERAL,
				ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
		}

		s_pMaterialGraphicsCommandList->End();

		signalValue++;
		RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&s_pMaterialGraphicsCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, s_pMaterialFence, signalValue - 1, s_pMaterialFence, signalValue);

		s_pMaterialFence->Wait(signalValue, UINT64_MAX);

		pMaterial->pAOMetallicRoughnessMap		= pCombinedMaterialTexture;
		pMaterial->pAOMetallicRoughnessMapView	= pCombinedMaterialTextureView;
		return RegisterTextureWithView(pCombinedMaterialTexture, pCombinedMaterialTextureView);
	}

	bool ResourceManager::UnloadMesh(GUID_Lambda guid)
	{
		auto meshIt = s_Meshes.find(guid);
		if (meshIt != s_Meshes.end())
		{
			SAFEDELETE(meshIt->second);
			s_Meshes.erase(meshIt);

			auto meshGUIDToNameIt = s_MeshGUIDsToNames.find(guid);

			if (meshGUIDToNameIt != s_MeshGUIDsToNames.end())
			{
				//Clean Mesh Name -> GUID
				auto meshNameToGUIDIt = s_MeshNamesToGUIDs.find(meshGUIDToNameIt->second);
				if (meshNameToGUIDIt != s_MeshNamesToGUIDs.end()) s_MeshNamesToGUIDs.erase(meshNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadMesh Failed at s_MeshNamesToGUIDs GUID: %d", guid);
					return false;
				}

				auto animationsIt = s_FileNamesToAnimationGUIDs.find(meshGUIDToNameIt->second);

				//Clean Mesh GUID -> Name
				s_MeshGUIDsToNames.erase(meshGUIDToNameIt);

				if (animationsIt != s_FileNamesToAnimationGUIDs.end())
				{
					bool result = true;

					for (GUID_Lambda animationGUID : animationsIt->second)
					{
						result = result && UnloadAnimation(animationGUID);
					}

					//Clean Mesh Name -> Animation GUID
					s_FileNamesToAnimationGUIDs.erase(animationsIt);

					if (!result)
					{
						LOG_ERROR("[ResourceManager]: UnloadMesh Failed at unloading some Animation GUID: %d", guid);
						return false;
					}
				}
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadMesh Failed at s_MeshGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadMesh Failed at s_Meshes GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadMaterial(GUID_Lambda guid)
	{
		auto materialIt = s_Materials.find(guid);
		if (materialIt != s_Materials.end())
		{
			D_LOG_WARNING("Deleted Material GUID: %d", guid);

			SAFEDELETE(materialIt->second);
			s_Materials.erase(materialIt);

			auto materialGUIDToNameIt = s_MaterialGUIDsToNames.find(guid);
			if (materialGUIDToNameIt != s_MaterialGUIDsToNames.end())
			{
				//Clean Material Name -> GUID
				auto materialNameToGUIDIt = s_MaterialNamesToGUIDs.find(materialGUIDToNameIt->second);
				if (materialNameToGUIDIt != s_MaterialNamesToGUIDs.end()) s_MaterialNamesToGUIDs.erase(materialNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadMaterial Failed at s_MaterialNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Material GUID -> Name
				s_MaterialGUIDsToNames.erase(materialGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadMaterial Failed at s_MaterialGUIDsToNames GUID: %d", guid);
				return false;
			}

			auto materialLoadConfigIt = s_MaterialLoadConfigurations.find(guid);
			if (materialLoadConfigIt != s_MaterialLoadConfigurations.end())
			{
				if (materialLoadConfigIt->second.AlbedoMapGUID					!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.AlbedoMapGUID);
				if (materialLoadConfigIt->second.NormalMapGUID					!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.NormalMapGUID);
				if (materialLoadConfigIt->second.AOMapGUID						!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.AOMapGUID);
				if (materialLoadConfigIt->second.MetallicMapGUID				!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.MetallicMapGUID);
				if (materialLoadConfigIt->second.RoughnessMapGUID				!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.RoughnessMapGUID);
				if (materialLoadConfigIt->second.AOMetallicRoughnessMapGUID		!= GUID_NONE) DecrementTextureMaterialRef(materialLoadConfigIt->second.AOMetallicRoughnessMapGUID);

				//Clean Material Load Config
				s_MaterialLoadConfigurations.erase(materialLoadConfigIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadMaterial Failed at s_MaterialLoadConfigurations GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadMaterial Failed at s_Materials GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadAnimation(GUID_Lambda guid)
	{
		auto animationIt = s_Animations.find(guid);
		if (animationIt != s_Animations.end())
		{
			D_LOG_WARNING("Deleted Animation GUID: %d", guid);

			SAFEDELETE(animationIt->second);
			s_Animations.erase(animationIt);

			auto animationGUIDToNameIt = s_AnimationGUIDsToNames.find(guid);
			if (animationGUIDToNameIt != s_AnimationGUIDsToNames.end())
			{
				//Clean Animation Name -> GUID
				auto animationNameToGUIDIt = s_AnimationNamesToGUIDs.find(animationGUIDToNameIt->second);
				if (animationNameToGUIDIt != s_AnimationNamesToGUIDs.end()) s_AnimationNamesToGUIDs.erase(animationNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadAnimation Failed at s_AnimationNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Animation GUID -> Name
				s_AnimationGUIDsToNames.erase(animationGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadAnimation Failed at s_AnimationGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadAnimation Failed at s_Animations GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadTexture(GUID_Lambda guid)
	{
		auto textureIt = s_Textures.find(guid);
		if (textureIt != s_Textures.end())
		{
			D_LOG_WARNING("Deleted Texture GUID: %d", guid);

			SAFEDELETE(textureIt->second);
			s_Textures.erase(textureIt);

			auto textureGUIDToNameIt = s_TextureGUIDsToNames.find(guid);
			if (textureGUIDToNameIt != s_TextureGUIDsToNames.end())
			{
				//Clean Texture Name -> GUID
				auto textureNameToGUIDIt = s_TextureNamesToGUIDs.find(textureGUIDToNameIt->second);
				if (textureNameToGUIDIt != s_TextureNamesToGUIDs.end()) s_TextureNamesToGUIDs.erase(textureNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadTexture Failed at s_TextureNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Texture GUID -> Name
				s_TextureGUIDsToNames.erase(textureGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadTexture Failed at s_TextureGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadTexture Failed at s_Textures GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadShader(GUID_Lambda guid)
	{
		auto shaderIt = s_Shaders.find(guid);
		if (shaderIt != s_Shaders.end())
		{
			D_LOG_WARNING("Deleted Shader GUID: %d", guid);

			SAFEDELETE(shaderIt->second);
			s_Shaders.erase(shaderIt);

			auto shaderGUIDToNameIt = s_ShaderGUIDsToNames.find(guid);
			if (shaderGUIDToNameIt != s_ShaderGUIDsToNames.end())
			{
				//Clean Shader Name -> GUID
				auto shaderNameToGUIDIt = s_ShaderNamesToGUIDs.find(shaderGUIDToNameIt->second);
				if (shaderNameToGUIDIt != s_ShaderNamesToGUIDs.end()) s_ShaderNamesToGUIDs.erase(shaderNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadShader Failed at s_ShaderNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Shader GUID -> Name
				s_ShaderGUIDsToNames.erase(shaderGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadShader Failed at s_ShaderGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadShader Failed at s_Shaders GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadSoundEffect3D(GUID_Lambda guid)
	{
		auto soundEffectIt = s_SoundEffects3D.find(guid);
		if (soundEffectIt != s_SoundEffects3D.end())
		{
			D_LOG_WARNING("Deleted 3D Sound Effect GUID: %d", guid);

			SAFEDELETE(soundEffectIt->second);
			s_SoundEffects3D.erase(soundEffectIt);

			auto soundEffectGUIDToNameIt = s_SoundEffect3DGUIDsToNames.find(guid);
			if (soundEffectGUIDToNameIt != s_SoundEffect3DGUIDsToNames.end())
			{
				//Clean Sound Effect Name -> GUID
				auto soundEffectNameToGUIDIt = s_SoundEffect3DNamesToGUIDs.find(soundEffectGUIDToNameIt->second);
				if (soundEffectNameToGUIDIt != s_SoundEffect3DNamesToGUIDs.end()) s_SoundEffect3DNamesToGUIDs.erase(soundEffectNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffect3DNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Sound Effect GUID -> Name
				s_SoundEffect3DGUIDsToNames.erase(soundEffectGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffect3DGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffects3D GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadSoundEffect2D(GUID_Lambda guid)
	{
		auto soundEffectIt = s_SoundEffects2D.find(guid);
		if (soundEffectIt != s_SoundEffects2D.end())
		{
			D_LOG_WARNING("Deleted 2D Sound Effect GUID: %d", guid);

			SAFEDELETE(soundEffectIt->second);
			s_SoundEffects2D.erase(soundEffectIt);

			auto soundEffectGUIDToNameIt = s_SoundEffect2DGUIDsToNames.find(guid);
			if (soundEffectGUIDToNameIt != s_SoundEffect2DGUIDsToNames.end())
			{
				//Clean Sound Effect Name -> GUID
				auto soundEffectNameToGUIDIt = s_SoundEffect2DNamesToGUIDs.find(soundEffectGUIDToNameIt->second);
				if (soundEffectNameToGUIDIt != s_SoundEffect2DNamesToGUIDs.end()) s_SoundEffect2DNamesToGUIDs.erase(soundEffectNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffect2DNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Sound Effect GUID -> Name
				s_SoundEffect2DGUIDsToNames.erase(soundEffectGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffect2DGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_SoundEffects2D GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::UnloadMusic(GUID_Lambda guid)
	{
		auto musicIt = s_Music.find(guid);
		if (musicIt != s_Music.end())
		{
			D_LOG_WARNING("Deleted 2D Sound Effect GUID: %d", guid);

			SAFEDELETE(musicIt->second);
			s_Music.erase(musicIt);

			auto musicGUIDToNameIt = s_MusicGUIDsToNames.find(guid);
			if (musicGUIDToNameIt != s_MusicGUIDsToNames.end())
			{
				//Clean Sound Effect Name -> GUID
				auto musicNameToGUIDIt = s_MusicNamesToGUIDs.find(musicGUIDToNameIt->second);
				if (musicNameToGUIDIt != s_MusicNamesToGUIDs.end()) s_MusicNamesToGUIDs.erase(musicNameToGUIDIt);
				else
				{
					LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_MusicNamesToGUIDs GUID: %d", guid);
					return false;
				}

				//Clean Sound Effect GUID -> Name
				s_MusicGUIDsToNames.erase(musicGUIDToNameIt);
			}
			else
			{
				LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_MusicGUIDsToNames GUID: %d", guid);
				return false;
			}
		}
		else if (s_UnloadedGUIDs.count(guid) == 0)
		{
			LOG_ERROR("[ResourceManager]: UnloadSoundEffect Failed at s_Music GUID: %d", guid);
			return false;
		}
		else
		{
			return true;
		}

		s_UnloadedGUIDs.insert(guid);
		return true;
	}

	bool ResourceManager::DecrementTextureMaterialRef(GUID_Lambda guid)
	{
		auto textureRefIt = s_TextureMaterialRefs.find(guid);
		if (textureRefIt != s_TextureMaterialRefs.end())
		{
			textureRefIt->second--;

			if (textureRefIt->second == 0)
			{
				//Clean Material Ref
				s_TextureMaterialRefs.erase(textureRefIt);

				return UnloadTexture(guid);
			}
		}
		else
		{
			LOG_ERROR("[ResourceManager]: Failed to DecrementTextureMaterialRef GUID: %d", guid);
			return false;
		}

		return true;
	}

	GUID_Lambda ResourceManager::GetMeshGUID(const String& name)
	{
		return GetGUID(s_MeshNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetMaterialGUID(const String& name)
	{
		return GetGUID(s_MaterialNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetAnimationGUID(const String& name)
	{
		return GetGUID(s_AnimationNamesToGUIDs, name);
	}

	bool ResourceManager::GetAnimationGUIDsFromMeshName(const String& name, TArray<GUID_Lambda>& guids)
	{
		if (auto it = s_FileNamesToAnimationGUIDs.find(name); it != s_FileNamesToAnimationGUIDs.end())
		{
			guids = it->second;
			return true;
		}

		return false;
	}

	GUID_Lambda ResourceManager::GetTextureGUID(const String& name)
	{
		return GetGUID(s_TextureNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetShaderGUID(const String& name)
	{
		return GetGUID(s_ShaderNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetSoundEffect3DGUID(const String& name)
	{
		return GetGUID(s_SoundEffect3DNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetSoundEffect2DGUID(const String& name)
	{
		return GetGUID(s_SoundEffect2DNamesToGUIDs, name);
	}

	Mesh* ResourceManager::GetMesh(GUID_Lambda guid)
	{
		auto it = s_Meshes.find(guid);
		if (it != s_Meshes.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	Material* ResourceManager::GetMaterial(GUID_Lambda guid)
	{
		auto it = s_Materials.find(guid);
		if (it != s_Materials.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	Animation* ResourceManager::GetAnimation(GUID_Lambda guid)
	{
		auto it = s_Animations.find(guid);
		if (it != s_Animations.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetClip called with invalid GUID %u", guid);
		return nullptr;
	}

	Texture* ResourceManager::GetTexture(GUID_Lambda guid)
	{
		auto it = s_Textures.find(guid);
		if (it != s_Textures.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	TextureView* ResourceManager::GetTextureView(GUID_Lambda guid)
	{
		auto it = s_TextureViews.find(guid);
		if (it != s_TextureViews.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetTextureView called with invalid GUID %u", guid);
		return nullptr;
	}

	Shader* ResourceManager::GetShader(GUID_Lambda guid)
	{
		auto it = s_Shaders.find(guid);
		if (it != s_Shaders.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetShader called with invalid GUID %u", guid);
		return nullptr;
	}

	ISoundEffect3D* ResourceManager::GetSoundEffect3D(GUID_Lambda guid)
	{
		auto it = s_SoundEffects3D.find(guid);
		if (it != s_SoundEffects3D.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetSoundEffect called with invalid GUID %u", guid);
		return nullptr;
	}

	ISoundEffect2D* ResourceManager::GetSoundEffect2D(GUID_Lambda guid)
	{
		auto it = s_SoundEffects2D.find(guid);
		if (it != s_SoundEffects2D.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetSoundEffect called with invalid GUID %u", guid);
		return nullptr;
	}

	IMusic* ResourceManager::GetMusic(GUID_Lambda guid)
	{
		auto it = s_Music.find(guid);
		if (it != s_Music.end())
		{
			return it->second;
		}

		D_LOG_WARNING("[ResourceManager]: GetMusic called with invalid GUID %u", guid);
		return nullptr;
	}

	bool ResourceManager::OnShaderRecompileEvent(const ShaderRecompileEvent& event)
	{
		UNREFERENCED_VARIABLE(event);

		for (auto it = s_Shaders.begin(); it != s_Shaders.end(); it++)
		{
			if (it->second != nullptr)
			{
				auto loadConfigIt = s_ShaderLoadConfigurations.find(it->first);

				if (loadConfigIt != s_ShaderLoadConfigurations.end())
				{
					Shader* pShader = ResourceLoader::LoadShaderFromFile(loadConfigIt->second.Filepath, loadConfigIt->second.Stage, loadConfigIt->second.Lang, loadConfigIt->second.pEntryPoint);
					if (pShader != nullptr)
					{
						SAFERELEASE(it->second);
						it->second = pShader;
					}
				}
			}
		}

		return true;
	}

	void ResourceManager::RegisterLoadedMaterialTexture(
		LoadedTexture* pLoadedTexture, 
		LoadedMaterial* pLoadedMaterial, 
		MaterialLoadDesc& materialLoadDescription, 
		TArray<TextureView*>& textureViewsToDelete)
	{
		TArray<LoadedMaterial*> loadedMaterials(1, pLoadedMaterial);
		TArray<MaterialLoadDesc> loadedMaterialDescriptions(1, materialLoadDescription);

		RegisterLoadedMaterialTexture(
			pLoadedTexture,
			loadedMaterials,
			loadedMaterialDescriptions,
			textureViewsToDelete);
	}

	void ResourceManager::RegisterLoadedMaterialTexture(
		LoadedTexture* pLoadedTexture,
		TArray<LoadedMaterial*>& loadedMaterials,
		TArray<MaterialLoadDesc>& materialLoadDescriptions,
		TArray<TextureView*>& textureViewsToDelete)
	{
		if (pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ALBEDO ||
			pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL)
		{
			GUID_Lambda guid = RegisterTexture(pLoadedTexture->pTexture);

			// RegisterLoadedTexture will create a TextureView for the texture, this needs to be registered in the correct materials
			for (uint32 j = 0; j < loadedMaterials.GetSize(); j++)
			{
				LoadedMaterial* pLoadedMaterial = loadedMaterials[j];
				MaterialLoadDesc& materialLoadDesc = materialLoadDescriptions[j];

				if (pLoadedMaterial->pAlbedoMap == pLoadedTexture)
				{
					pLoadedMaterial->pAlbedoMapView = s_TextureViews[guid];
					s_TextureMaterialRefs[guid]++;
					materialLoadDesc.AlbedoMapGUID = guid;
				}

				if (pLoadedMaterial->pNormalMap == pLoadedTexture)
				{
					pLoadedMaterial->pNormalMapView = s_TextureViews[guid];
					s_TextureMaterialRefs[guid]++;
					materialLoadDesc.NormalMapGUID = guid;
				}
			}
		}
			
		if (pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_AO			||
			pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_METALLIC	||
			pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ROUGHNESS	||
			pLoadedTexture->Flags & FLoadedTextureFlag::LOADED_TEXTURE_FLAG_METALLIC_ROUGHNESS)
		{
			TextureViewDesc textureViewDesc = {};
			textureViewDesc.DebugName		= pLoadedTexture->pTexture->GetDesc().DebugName + " Texture View";
			textureViewDesc.pTexture		= pLoadedTexture->pTexture;
			textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
			textureViewDesc.Format			= pLoadedTexture->pTexture->GetDesc().Format;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= pLoadedTexture->pTexture->GetDesc().Miplevels;
			textureViewDesc.ArrayCount		= pLoadedTexture->pTexture->GetDesc().ArrayCount;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;

			TextureView* pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
			textureViewsToDelete.PushBack(pTextureView);

			//Registered in the correct materials
			for (uint32 j = 0; j < loadedMaterials.GetSize(); j++)
			{
				LoadedMaterial* pLoadedMaterial = loadedMaterials[j];

				if (pLoadedMaterial->pAmbientOcclusionMap == pLoadedTexture)
					pLoadedMaterial->pAmbientOcclusionMapView = pTextureView;

				if (pLoadedMaterial->pMetallicMap == pLoadedTexture)
					pLoadedMaterial->pMetallicMapView = pTextureView;

				if (pLoadedMaterial->pRoughnessMap == pLoadedTexture)
					pLoadedMaterial->pRoughnessMapView = pTextureView;

				if (pLoadedMaterial->pMetallicRoughnessMap == pLoadedTexture)
					pLoadedMaterial->pMetallicRoughnessMapView = pTextureView;
			}
		}
	}

	GUID_Lambda ResourceManager::RegisterLoadedMaterial(const String& name, LoadedMaterial* pLoadedMaterial, MaterialLoadDesc& materialLoadConfig)
	{
		Material* pMaterialToBeRegistered = DBG_NEW Material();
		pMaterialToBeRegistered->Properties		= pLoadedMaterial->Properties;
		pMaterialToBeRegistered->pAlbedoMap		= pLoadedMaterial->pAlbedoMap != nullptr ? pLoadedMaterial->pAlbedoMap->pTexture : nullptr;
		pMaterialToBeRegistered->pNormalMap		= pLoadedMaterial->pNormalMap != nullptr ? pLoadedMaterial->pNormalMap->pTexture : nullptr;
		pMaterialToBeRegistered->pAlbedoMapView	= pLoadedMaterial->pAlbedoMapView;
		pMaterialToBeRegistered->pNormalMapView	= pLoadedMaterial->pNormalMapView;

		//If AO, Metallic & Roughness are all nullptr we can set default
		if (pLoadedMaterial->pAmbientOcclusionMap	== nullptr &&
			pLoadedMaterial->pMetallicMap			== nullptr &&
			pLoadedMaterial->pRoughnessMap			== nullptr &&
			pLoadedMaterial->pMetallicRoughnessMap	== nullptr)
		{
			pMaterialToBeRegistered->pAOMetallicRoughnessMap		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pMaterialToBeRegistered->pAOMetallicRoughnessMapView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		}
		else
		{
			Texture*		pDefaultColorTexture		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			TextureView*	pDefaultColorTextureView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			bool useSeperateChannels = pLoadedMaterial->pMetallicRoughnessMap == nullptr;
			GUID_Lambda aoMetallicRoughnessGUID = GUID_NONE;

			if (useSeperateChannels)
			{
				aoMetallicRoughnessGUID = CombineMaterialTextures(
					name + " Combined Material Texture",
					pMaterialToBeRegistered,
					pLoadedMaterial->pAmbientOcclusionMap	!= nullptr ? pLoadedMaterial->pAmbientOcclusionMap->pTexture	: pDefaultColorTexture,
					pLoadedMaterial->pMetallicMap			!= nullptr ? pLoadedMaterial->pMetallicMap->pTexture			: pDefaultColorTexture,
					pLoadedMaterial->pRoughnessMap			!= nullptr ? pLoadedMaterial->pRoughnessMap->pTexture			: pDefaultColorTexture,
					nullptr,
					pLoadedMaterial->pAmbientOcclusionMap	!= nullptr ? pLoadedMaterial->pAmbientOcclusionMapView			: pDefaultColorTextureView,
					pLoadedMaterial->pMetallicMap			!= nullptr ? pLoadedMaterial->pMetallicMapView					: pDefaultColorTextureView,
					pLoadedMaterial->pRoughnessMap			!= nullptr ? pLoadedMaterial->pRoughnessMapView					: pDefaultColorTextureView,
					nullptr);
			}
			else
			{
				aoMetallicRoughnessGUID = CombineMaterialTextures(
					name + " Combined Material Texture",
					pMaterialToBeRegistered,
					pLoadedMaterial->pAmbientOcclusionMap	!= nullptr ? pLoadedMaterial->pAmbientOcclusionMap->pTexture	: pDefaultColorTexture,
					nullptr,
					nullptr,
					pLoadedMaterial->pMetallicRoughnessMap	!= nullptr ? pLoadedMaterial->pMetallicRoughnessMap->pTexture	: pDefaultColorTexture,
					pLoadedMaterial->pAmbientOcclusionMap	!= nullptr ? pLoadedMaterial->pAmbientOcclusionMapView			: pDefaultColorTextureView,
					nullptr,
					nullptr,
					pLoadedMaterial->pMetallicRoughnessMap	!= nullptr ? pLoadedMaterial->pMetallicRoughnessMapView			: pDefaultColorTextureView);
			}

			s_TextureMaterialRefs[aoMetallicRoughnessGUID]++;
			materialLoadConfig.NormalMapGUID = aoMetallicRoughnessGUID;
		}

		GUID_Lambda guid = RegisterMaterial(name, pMaterialToBeRegistered);
		s_MaterialLoadConfigurations[guid] = materialLoadConfig;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterMesh(const String& name, Mesh* pResource)
	{
		VALIDATE(pResource != nullptr);

		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedResource = nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedResource			= &s_Meshes[guid]; //Creates new entry if not existing
			s_MeshGUIDsToNames[guid]	= name;
			s_MeshNamesToGUIDs[name]	= guid;
		}

		(*ppMappedResource) = pResource;
		return guid;
	}

	GUID_Lambda ResourceManager::RegisterMaterial(const String& name, Material* pResource)
	{
		VALIDATE(pResource != nullptr);

		GUID_Lambda guid = GUID_NONE;
		Material** ppMappedResource = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedResource				= &s_Materials[guid]; //Creates new entry if not existing
			s_MaterialGUIDsToNames[guid]	= name;
			s_MaterialNamesToGUIDs[name]	= guid;
		}

		pResource->pAlbedoMap					= pResource->pAlbedoMap						!= nullptr ? pResource->pAlbedoMap					: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pNormalMap					= pResource->pNormalMap						!= nullptr ? pResource->pNormalMap					: s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		pResource->pAOMetallicRoughnessMap		= pResource->pAOMetallicRoughnessMap		!= nullptr ? pResource->pAOMetallicRoughnessMap		: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		pResource->pAlbedoMapView				= pResource->pAlbedoMapView					!= nullptr ? pResource->pAlbedoMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pNormalMapView				= pResource->pNormalMapView					!= nullptr ? pResource->pNormalMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		pResource->pAOMetallicRoughnessMapView	= pResource->pAOMetallicRoughnessMapView	!= nullptr ? pResource->pAOMetallicRoughnessMapView	: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		(*ppMappedResource) = pResource;
		return guid;
	}

	GUID_Lambda ResourceManager::RegisterAnimation(const String& name, Animation* pAnimation)
	{
		VALIDATE(pAnimation != nullptr);

		GUID_Lambda guid = GUID_NONE;
		Animation** ppMappedResource = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedResource				= &s_Animations[guid]; //Creates new entry if not existing
			s_AnimationGUIDsToNames[guid]	= name;
			s_AnimationNamesToGUIDs[name]	= guid;
		}

		(*ppMappedResource) = pAnimation;
		return guid;
	}

	GUID_Lambda ResourceManager::RegisterTexture(Texture* pTexture)
	{
		VALIDATE(pTexture != nullptr);

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= pTexture->GetDesc().DebugName + " Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= pTexture->GetDesc().Format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		TextureView* pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		return RegisterTextureWithView(pTexture, pTextureView);
	}

	GUID_Lambda ResourceManager::RegisterTextureWithView(Texture* pTexture, TextureView* pTextureView)
	{
		GUID_Lambda		guid				= GUID_NONE;
		Texture**		ppMappedTexture		= nullptr;
		TextureView**	ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid				= s_NextFreeGUID++;
			ppMappedTexture		= &s_Textures[guid];		//Creates new entry if not existing
			ppMappedTextureView	= &s_TextureViews[guid];	//Creates new entry if not existing
			s_TextureGUIDsToNames[guid] = pTexture->GetDesc().DebugName;
			s_TextureNamesToGUIDs[pTexture->GetDesc().DebugName] = guid;
		}

		(*ppMappedTexture)		= pTexture;
		(*ppMappedTextureView)	= pTextureView;

		return guid;
	}

	GUID_Lambda ResourceManager::GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name)
	{
		auto guidIt = namesToGUIDs.find(name);
		if (guidIt != namesToGUIDs.end())
		{
			return guidIt->second;
		}

		if (name.length() > 0)
		{
			LOG_ERROR("[ResourceManager]: Resource \"%s\" could not be fouund in ResourceManager", name.c_str());
		}

		return GUID_NONE;
	}

	void ResourceManager::InitMaterialCreation()
	{
		// Create Command Lists
		{
			s_pMaterialComputeCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Combine Material Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
			s_pMaterialGraphicsCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Combine Material Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			CommandListDesc commandListDesc = { };
			commandListDesc.DebugName			= "Compute Command List";
			commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			s_pMaterialComputeCommandList = RenderAPI::GetDevice()->CreateCommandList(s_pMaterialComputeCommandAllocator, &commandListDesc);

			commandListDesc.DebugName = "Graphics Command List";
			s_pMaterialGraphicsCommandList = RenderAPI::GetDevice()->CreateCommandList(s_pMaterialGraphicsCommandAllocator, &commandListDesc);
		}

		//Create Fence
		{
			FenceDesc fenceDesc = {};
			fenceDesc.DebugName		= "CombineMaterials Fence";
			fenceDesc.InitalValue	= 0;
			s_pMaterialFence = RenderAPI::GetDevice()->CreateFence(&fenceDesc);
		}

		//Create Descriptor Heap
		{
			DescriptorHeapInfo descriptorCountDesc = { };
			descriptorCountDesc.SamplerDescriptorCount	= 4;

			DescriptorHeapDesc descriptorHeapDesc = { };
			descriptorHeapDesc.DebugName				= "CombineMaterial Descriptor";
			descriptorHeapDesc.DescriptorSetCount		= 1;
			descriptorHeapDesc.DescriptorCount			= descriptorCountDesc;

			s_pMaterialDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		}

		//Create Pipeline Layout, Descriptor Set & Pipeline State
		{
			TSharedRef<Sampler> sampler = MakeSharedRef(Sampler::GetLinearSampler());

			DescriptorBindingDesc outputTextureBinding	= { };
			outputTextureBinding.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
			outputTextureBinding.DescriptorCount		= 1;
			outputTextureBinding.Binding				= 0;
			outputTextureBinding.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc aoBinding				= { };
			aoBinding.DescriptorType					= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			aoBinding.DescriptorCount					= 1;
			aoBinding.Binding							= 1;
			aoBinding.ShaderStageMask					= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
			aoBinding.ImmutableSamplers					= { sampler };

			//This binding is also used for combined metallic/roughness
			DescriptorBindingDesc metallicBinding		= { };
			metallicBinding.DescriptorType				= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			metallicBinding.DescriptorCount				= 1;
			metallicBinding.Binding						= 2;
			metallicBinding.ShaderStageMask				= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
			metallicBinding.ImmutableSamplers			= { sampler };

			DescriptorBindingDesc roughnessBinding		= { };
			roughnessBinding.DescriptorType				= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			roughnessBinding.DescriptorCount			= 1;
			roughnessBinding.Binding					= 3;
			roughnessBinding.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
			roughnessBinding.ImmutableSamplers			= { sampler };

			DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
			descriptorSetLayoutDesc.DescriptorBindings = 
			{ 
				outputTextureBinding, 
				aoBinding,
				metallicBinding,
				roughnessBinding
			};

			PipelineLayoutDesc pPipelineLayoutDesc = { };
			pPipelineLayoutDesc.DebugName				= "Combined Material Pipeline Layout";
			pPipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc };

			s_pMaterialPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pPipelineLayoutDesc);

			s_pMaterialDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Combine Material Descriptor Set", s_pMaterialPipelineLayout, 0, s_pMaterialDescriptorHeap);

			// Create Shaders
			s_AllChannelsSeperateMaterialShaderGUID = LoadShaderFromFile("Material/CombineMaterialAllSeperate.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");
			s_AOSeperateMetRoughCombinedMaterialShaderGUID = LoadShaderFromFile("Material/CombineMaterialAOSeperateMetRoughCombined.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");

			{
				ComputePipelineStateDesc computePipelineStateDesc = { };
				computePipelineStateDesc.DebugName			= "All Channels Seperate, Material Pipeline State";
				computePipelineStateDesc.pPipelineLayout	= s_pMaterialPipelineLayout;
				computePipelineStateDesc.Shader				= { .pShader = s_Shaders[s_AllChannelsSeperateMaterialShaderGUID] };

				s_pAllChannelsSeperateMaterialPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&computePipelineStateDesc);
			}

			{
				ComputePipelineStateDesc computePipelineStateDesc = { };
				computePipelineStateDesc.DebugName			= "AO Seperate, Metallic/Roughness Combined, Material Pipeline State";
				computePipelineStateDesc.pPipelineLayout	= s_pMaterialPipelineLayout;
				computePipelineStateDesc.Shader				= { .pShader = s_Shaders[s_AOSeperateMetRoughCombinedMaterialShaderGUID] };

				s_pAOSeperateMetRoughCombinedMaterialPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&computePipelineStateDesc);
			}
		}
	}

	void ResourceManager::InitDefaultResources()
	{
		s_Meshes[GUID_NONE]			= nullptr;
		s_Materials[GUID_NONE]		= nullptr;
		s_Textures[GUID_NONE]		= nullptr;
		s_TextureViews[GUID_NONE]	= nullptr;
		s_Shaders[GUID_NONE]		= nullptr;
		s_SoundEffects3D[GUID_NONE]	= nullptr;
		s_SoundEffects2D[GUID_NONE]	= nullptr;
		s_Music[GUID_NONE]			= nullptr;

		{
			s_MeshNamesToGUIDs["Quad"]			= GUID_MESH_QUAD;
			s_Meshes[GUID_MESH_QUAD]			= MeshFactory::CreateQuad();
		}

		{
			byte defaultColor[4]				= { 255, 255, 255, 255 };
			byte defaultNormal[4]				= { 127, 127, 255,   0 };
			byte defaultMask[4]					= {	  0,   0,   0, 255 };
			void* pDefaultColor					= (void*)defaultColor;
			void* pDefaultNormal				= (void*)defaultNormal;
			void* pDefaultMask					= (void*)defaultMask;
			Texture* pDefaultColorMap			= ResourceLoader::LoadTextureArrayFromMemory("Default Color Map", &pDefaultColor, 1, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, false, true);
			Texture* pDefaultNormalMap			= ResourceLoader::LoadTextureArrayFromMemory("Default Normal Map", &pDefaultNormal, 1, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, false, true);
			Texture* pDefaultMaskMap			= ResourceLoader::LoadTextureArrayFromMemory("Default Mask Map", &pDefaultMask, 1, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, false, true);

			s_TextureNamesToGUIDs[pDefaultColorMap->GetDesc().DebugName]	= GUID_TEXTURE_DEFAULT_COLOR_MAP;
			s_TextureNamesToGUIDs[pDefaultNormalMap->GetDesc().DebugName]	= GUID_TEXTURE_DEFAULT_NORMAL_MAP;
			s_TextureNamesToGUIDs[pDefaultMaskMap->GetDesc().DebugName]		= GUID_TEXTURE_DEFAULT_MASK_MAP;
			s_TextureGUIDsToNames[GUID_TEXTURE_DEFAULT_COLOR_MAP]			= pDefaultColorMap->GetDesc().DebugName;
			s_TextureGUIDsToNames[GUID_TEXTURE_DEFAULT_NORMAL_MAP]			= pDefaultNormalMap->GetDesc().DebugName;
			s_TextureGUIDsToNames[GUID_TEXTURE_DEFAULT_MASK_MAP]			= pDefaultMaskMap->GetDesc().DebugName;

			s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP]		= pDefaultColorMap;
			s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP]		= pDefaultNormalMap;
			s_Textures[GUID_TEXTURE_DEFAULT_MASK_MAP]		= pDefaultMaskMap;

			TextureViewDesc defaultColorMapViewDesc = {};
			defaultColorMapViewDesc.DebugName		= "Default Color Map View";
			defaultColorMapViewDesc.pTexture		= pDefaultColorMap;
			defaultColorMapViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
			defaultColorMapViewDesc.Format			= pDefaultColorMap->GetDesc().Format;
			defaultColorMapViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			defaultColorMapViewDesc.MiplevelCount	= pDefaultColorMap->GetDesc().Miplevels;
			defaultColorMapViewDesc.ArrayCount		= pDefaultColorMap->GetDesc().ArrayCount;
			defaultColorMapViewDesc.Miplevel		= 0;
			defaultColorMapViewDesc.ArrayIndex		= 0;

			TextureViewDesc defaultNormalMapViewDesc = {};
			defaultNormalMapViewDesc.DebugName		= "Default Normal Map View";
			defaultNormalMapViewDesc.pTexture		= pDefaultNormalMap;
			defaultNormalMapViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
			defaultNormalMapViewDesc.Format			= pDefaultNormalMap->GetDesc().Format;
			defaultNormalMapViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			defaultNormalMapViewDesc.MiplevelCount	= pDefaultNormalMap->GetDesc().Miplevels;
			defaultNormalMapViewDesc.ArrayCount		= pDefaultNormalMap->GetDesc().ArrayCount;
			defaultNormalMapViewDesc.Miplevel		= 0;
			defaultNormalMapViewDesc.ArrayIndex		= 0;

			TextureViewDesc defaultMaskMapViewDesc = {};
			defaultMaskMapViewDesc.DebugName = "Default Mask Map View";
			defaultMaskMapViewDesc.pTexture = pDefaultMaskMap;
			defaultMaskMapViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
			defaultMaskMapViewDesc.Format = pDefaultMaskMap->GetDesc().Format;
			defaultMaskMapViewDesc.Type = ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			defaultMaskMapViewDesc.MiplevelCount = pDefaultMaskMap->GetDesc().Miplevels;
			defaultMaskMapViewDesc.ArrayCount = pDefaultMaskMap->GetDesc().ArrayCount;
			defaultMaskMapViewDesc.Miplevel = 0;
			defaultMaskMapViewDesc.ArrayIndex = 0;

			s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP]		= RenderAPI::GetDevice()->CreateTextureView(&defaultColorMapViewDesc);
			s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP]		= RenderAPI::GetDevice()->CreateTextureView(&defaultNormalMapViewDesc);
			s_TextureViews[GUID_TEXTURE_DEFAULT_MASK_MAP]		= RenderAPI::GetDevice()->CreateTextureView(&defaultMaskMapViewDesc);
		}

		{
			Material* pDefaultMaterial = DBG_NEW Material();
			pDefaultMaterial->pAlbedoMap					= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMap					= s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAOMetallicRoughnessMap		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			pDefaultMaterial->pAlbedoMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAOMetallicRoughnessMapView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			s_MaterialNamesToGUIDs["Default Material"]		= GUID_MATERIAL_DEFAULT;
			s_MaterialGUIDsToNames[GUID_MATERIAL_DEFAULT]	= "Default Material";
			s_Materials[GUID_MATERIAL_DEFAULT] = pDefaultMaterial;
		}
	}

	void ResourceManager::ReleaseMaterialCreation()
	{
		s_pAllChannelsSeperateMaterialPipelineState->Release();
		s_pAOSeperateMetRoughCombinedMaterialPipelineState->Release();
		s_pMaterialDescriptorSet->Release();
		s_pMaterialDescriptorHeap->Release();
		s_pMaterialPipelineLayout->Release();
		s_pMaterialFence->Release();
		s_pMaterialComputeCommandList->Release();
		s_pMaterialGraphicsCommandList->Release();
		s_pMaterialComputeCommandAllocator->Release();
		s_pMaterialGraphicsCommandAllocator->Release();
	}
}
