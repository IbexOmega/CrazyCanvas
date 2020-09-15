#include "Resources/ResourceManager.h"

#include "Rendering/Core/API/TextureView.h"

#include "Log/Log.h"

#include "Rendering/RenderSystem.h"

#include "Containers/TUniquePtr.h"

#include <utility>
#include <unordered_set>

#define SAFEDELETE_ALL(map)     for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); } map.clear()
#define SAFERELEASE_ALL(map)    for (auto it = map.begin(); it != map.end(); it++) { SAFERELEASE(it->second); } map.clear()

namespace LambdaEngine
{
	GUID_Lambda												ResourceManager::s_NextFreeGUID = SMALLEST_UNRESERVED_GUID;

	std::unordered_map<String, GUID_Lambda>					ResourceManager::s_MeshNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>					ResourceManager::s_MaterialNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>					ResourceManager::s_TextureNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>					ResourceManager::s_ShaderNamesToGUIDs;
	std::unordered_map<String, GUID_Lambda>					ResourceManager::s_SoundEffectNamesToGUIDs;

	std::unordered_map<GUID_Lambda, Mesh*>					ResourceManager::s_Meshes;
	std::unordered_map<GUID_Lambda, Material*>				ResourceManager::s_Materials;
	std::unordered_map<GUID_Lambda, Texture*>				ResourceManager::s_Textures;
	std::unordered_map<GUID_Lambda, TextureView*>			ResourceManager::s_TextureViews;
	std::unordered_map<GUID_Lambda, Shader*>				ResourceManager::s_Shaders;
	std::unordered_map<GUID_Lambda, ISoundEffect3D*>		ResourceManager::s_SoundEffects;

	std::unordered_map<GUID_Lambda, ResourceManager::ShaderLoadDesc>		ResourceManager::s_ShaderLoadConfigurations;

	bool ResourceManager::Init()
	{
		InitDefaultResources();

		return true;
	}

	bool ResourceManager::Release()
	{
		SAFEDELETE_ALL(s_Meshes);
		SAFEDELETE_ALL(s_Materials);
		SAFERELEASE_ALL(s_Textures);
		SAFERELEASE_ALL(s_TextureViews);
		SAFERELEASE_ALL(s_Shaders);
		SAFEDELETE_ALL(s_SoundEffects);

		return true;
	}

	bool ResourceManager::LoadSceneFromFile(const String& filename, TArray<GameObject>& result)
	{
		TArray<GameObject> sceneLocalGameObjects;
		TArray<Mesh*> meshes;
		TArray<Material*> materials;
		TArray<Texture*> textures;

		if (!ResourceLoader::LoadSceneFromFile(SCENE_DIR + filename, sceneLocalGameObjects, meshes, materials, textures))
		{
			return false;
		}

		result = sceneLocalGameObjects;
		for (uint32 i = 0; i < textures.GetSize(); i++)
		{
			Texture* pTexture = textures[i];

			GUID_Lambda guid = RegisterLoadedTexture(pTexture);

			// RegisterLoadedTexture will create a TextureView for the texture, this needs to be registered in the correct materials
			for (uint32 j = 0; j < materials.GetSize(); j++)
			{
				Material* pMaterial = materials[j];
				if (pMaterial->pAlbedoMap == pTexture)
					pMaterial->pAlbedoMapView = s_TextureViews[guid];

				if (pMaterial->pNormalMap == pTexture)
					pMaterial->pNormalMapView = s_TextureViews[guid];

				if (pMaterial->pAmbientOcclusionMap == pTexture)
					pMaterial->pAmbientOcclusionMapView = s_TextureViews[guid];

				if (pMaterial->pMetallicMap == pTexture)
					pMaterial->pMetallicMapView = s_TextureViews[guid];

				if (pMaterial->pRoughnessMap == pTexture)
					pMaterial->pRoughnessMapView = s_TextureViews[guid];
			}
		}

		for (uint32 i = 0; i < meshes.GetSize(); i++)
		{
			GenerateMeshlets(meshes[i]);

			GUID_Lambda guid = RegisterLoadedMesh("Scene Mesh " + std::to_string(i), meshes[i]);
			for (uint32 g = 0; g < sceneLocalGameObjects.GetSize(); g++)
			{
				if (sceneLocalGameObjects[g].Mesh == i)
				{
					result[g].Mesh = guid;
				}
			}
		}

		for (uint32 i = 0; i < materials.GetSize(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMaterial("Scene Material " + std::to_string(i), materials[i]);
			for (uint32 g = 0; g < sceneLocalGameObjects.GetSize(); g++)
			{
				if (sceneLocalGameObjects[g].Material == i)
				{
					result[g].Material = guid;
				}
			}
		}

		for (uint32 g = 0; g < sceneLocalGameObjects.GetSize(); g++)
		{
			if (sceneLocalGameObjects[g].Mesh >= meshes.GetSize())
			{
				LOG_ERROR("[ResourceManager]: GameObject %u in Scene %s has no Mesh", g, filename.c_str());
			}

			if (sceneLocalGameObjects[g].Material >= materials.GetSize())
			{
				result[g].Material = GUID_MATERIAL_DEFAULT;
				LOG_WARNING("[ResourceManager]: GameObject %u in Scene %s has no Material, default Material assigned", g, filename.c_str());
			}
		}

		return true;
	}

	GUID_Lambda ResourceManager::LoadMeshFromFile(const String& filename)
	{
		auto loadedMeshGUID = s_MeshNamesToGUIDs.find(filename);
		if (loadedMeshGUID != s_MeshNamesToGUIDs.end())
			return loadedMeshGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedMesh = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedMesh = &s_Meshes[guid]; //Creates new entry if not existing
			s_MeshNamesToGUIDs[filename] = guid;
		}

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromFile(MESH_DIR + filename);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadMeshFromMemory(const String& name, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		auto loadedMeshGUID = s_MeshNamesToGUIDs.find(name);
		if (loadedMeshGUID != s_MeshNamesToGUIDs.end())
			return loadedMeshGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedMesh = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedMesh = &s_Meshes[guid]; //Creates new entry if not existing
			s_MeshNamesToGUIDs[name] = guid;
		}

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromMemory(pVertices, numVertices, pIndices, numIndices);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadMaterialFromMemory(const String& name, GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties)
	{
		auto loadedMaterialGUID = s_MaterialNamesToGUIDs.find(name);
		if (loadedMaterialGUID != s_MaterialNamesToGUIDs.end())
			return loadedMaterialGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Material** ppMappedMaterial = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedMaterial = &s_Materials[guid]; //Creates new entry if not existing
			s_MaterialNamesToGUIDs[name] = guid;
		}

		(*ppMappedMaterial) = DBG_NEW Material();

		Texture* pAlbedoMap = albedoMap != GUID_NONE ? s_Textures[albedoMap] : s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pNormalMap = normalMap != GUID_NONE ? s_Textures[normalMap] : s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		Texture* pAmbientOcclusionMap = ambientOcclusionMap != GUID_NONE ? s_Textures[ambientOcclusionMap] : s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pMetallicMap = metallicMap != GUID_NONE ? s_Textures[metallicMap] : s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pRoughnessMap = roughnessMap != GUID_NONE ? s_Textures[roughnessMap] : s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		TextureView* pAlbedoMapView = albedoMap != GUID_NONE ? s_TextureViews[albedoMap] : s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pNormalMapView = normalMap != GUID_NONE ? s_TextureViews[normalMap] : s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		TextureView* pAmbientOcclusionMapView = ambientOcclusionMap != GUID_NONE ? s_TextureViews[ambientOcclusionMap] : s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pMetallicMapView = metallicMap != GUID_NONE ? s_TextureViews[metallicMap] : s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pRoughnessMapView = roughnessMap != GUID_NONE ? s_TextureViews[roughnessMap] : s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		(*ppMappedMaterial)->Properties = properties;

		(*ppMappedMaterial)->pAlbedoMap = pAlbedoMap;
		(*ppMappedMaterial)->pNormalMap = pNormalMap;
		(*ppMappedMaterial)->pAmbientOcclusionMap = pAmbientOcclusionMap;
		(*ppMappedMaterial)->pMetallicMap = pMetallicMap;
		(*ppMappedMaterial)->pRoughnessMap = pRoughnessMap;

		(*ppMappedMaterial)->pAlbedoMapView = pAlbedoMapView;
		(*ppMappedMaterial)->pNormalMapView = pNormalMapView;
		(*ppMappedMaterial)->pAmbientOcclusionMapView = pAmbientOcclusionMapView;
		(*ppMappedMaterial)->pMetallicMapView = pMetallicMapView;
		(*ppMappedMaterial)->pRoughnessMapView = pRoughnessMapView;

		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
			return loadedTextureGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Texture** ppMappedTexture = nullptr;
		TextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureNamesToGUIDs[name] = guid;
		}

		Texture* pTexture = ResourceLoader::LoadTextureArrayFromFile(name, TEXTURE_DIR, pFilenames, count, format, generateMips);

		(*ppMappedTexture) = pTexture;

		TextureDesc textureDesc = pTexture->GetDesc();

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName = name + " Texture View";
		textureViewDesc.pTexture = pTexture;
		textureViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format = format;
		textureViewDesc.Type = textureDesc.ArrayCount > 1 ? ETextureViewType::TEXTURE_VIEW_TYPE_2D_ARRAY : ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount = textureDesc.Miplevels;
		textureViewDesc.ArrayCount = textureDesc.ArrayCount;
		textureViewDesc.Miplevel = 0;
		textureViewDesc.ArrayIndex = 0;

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadCubeTexturesArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
			return loadedTextureGUID->second;

		uint32 textureCount = count * 6U;

		GUID_Lambda guid = GUID_NONE;
		Texture** ppMappedTexture = nullptr;
		TextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureNamesToGUIDs[name] = guid;
		}

		Texture* pTexture = ResourceLoader::LoadCubeTexturesArrayFromFile(name, TEXTURE_DIR, pFilenames, textureCount, format, generateMips);

		(*ppMappedTexture) = pTexture;

		TextureDesc textureDesc = pTexture->GetDesc();

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName = name + " Texture View";
		textureViewDesc.pTexture = pTexture;
		textureViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format = format;
		textureViewDesc.Type = count > 1 ? ETextureViewType::TEXTURE_VIEW_TYPE_CUBE_ARRAY : ETextureViewType::TEXTURE_VIEW_TYPE_CUBE;
		textureViewDesc.MiplevelCount = textureDesc.Miplevels;
		textureViewDesc.ArrayCount = textureDesc.ArrayCount;
		textureViewDesc.Miplevel = 0;
		textureViewDesc.ArrayIndex = 0;

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureFromFile(const String& filename, EFormat format, bool generateMips)
	{
		return LoadTextureArrayFromFile(filename, &filename, 1, format, generateMips);
	}

	GUID_Lambda ResourceManager::LoadTextureFromMemory(const String& name, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		auto loadedTextureGUID = s_TextureNamesToGUIDs.find(name);
		if (loadedTextureGUID != s_TextureNamesToGUIDs.end())
			return loadedTextureGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Texture** ppMappedTexture = nullptr;
		TextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureNamesToGUIDs[name] = guid;
		}

		Texture* pTexture = ResourceLoader::LoadTextureArrayFromMemory(name, &pData, 1, width, height, format, usageFlags, generateMips);

		(*ppMappedTexture) = pTexture;

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName = name + " Texture View";
		textureViewDesc.pTexture = pTexture;
		textureViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format = format;
		textureViewDesc.Type = ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount = pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount = pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel = 0;
		textureViewDesc.ArrayIndex = 0;

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadShaderFromFile(const String& filename, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint)
	{
		auto loadedShaderGUID = s_ShaderNamesToGUIDs.find(filename);
		if (loadedShaderGUID != s_ShaderNamesToGUIDs.end())
			return loadedShaderGUID->second;

		GUID_Lambda guid = GUID_NONE;
		Shader** ppMappedShader = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedShader = &s_Shaders[guid]; //Creates new entry if not existing
			s_ShaderNamesToGUIDs[filename] = guid;
		}

		String filepath = SHADER_DIR + filename;

		ShaderLoadDesc loadDesc = {};
		loadDesc.Filepath = filepath;
		loadDesc.Stage = stage;
		loadDesc.Lang = lang;
		loadDesc.pEntryPoint = pEntryPoint;

		s_ShaderLoadConfigurations[guid] = loadDesc;

		(*ppMappedShader) = ResourceLoader::LoadShaderFromFile(filepath, stage, lang, pEntryPoint);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadSoundEffectFromFile(const String& filename)
	{
		auto loadedSoundEffectGUID = s_SoundEffectNamesToGUIDs.find(filename);
		if (loadedSoundEffectGUID != s_SoundEffectNamesToGUIDs.end())
			return loadedSoundEffectGUID->second;

		GUID_Lambda guid = GUID_NONE;
		ISoundEffect3D** ppMappedSoundEffect = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedSoundEffect = &s_SoundEffects[guid]; //Creates new entry if not existing
			s_SoundEffectNamesToGUIDs[filename] = guid;
		}

		(*ppMappedSoundEffect) = ResourceLoader::LoadSoundEffectFromFile(SOUND_DIR + filename);

		return guid;
	}

	void ResourceManager::ReloadAllShaders()
	{
		for (auto it = s_Shaders.begin(); it != s_Shaders.end(); it++)
		{
			if (it->second != nullptr)
			{
				ShaderLoadDesc loadDesc = s_ShaderLoadConfigurations[it->first];

				Shader* pShader = ResourceLoader::LoadShaderFromFile(loadDesc.Filepath, loadDesc.Stage, loadDesc.Lang, loadDesc.pEntryPoint);

				if (pShader != nullptr)
				{
					SAFERELEASE(it->second);
					it->second = pShader;
				}
			}
		}
	}

	GUID_Lambda ResourceManager::GetMeshGUID(const String& name)
	{
		return GetGUID(s_MeshNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetMaterialGUID(const String& name)
	{
		return GetGUID(s_MaterialNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetTextureGUID(const String& name)
	{
		return GetGUID(s_TextureNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetShaderGUID(const String& name)
	{
		return GetGUID(s_ShaderNamesToGUIDs, name);
	}

	GUID_Lambda ResourceManager::GetSoundEffectGUID(const String& name)
	{
		return GetGUID(s_SoundEffectNamesToGUIDs, name);
	}

	Mesh* ResourceManager::GetMesh(GUID_Lambda guid)
	{
		auto it = s_Meshes.find(guid);

		if (it != s_Meshes.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	Material* ResourceManager::GetMaterial(GUID_Lambda guid)
	{
		auto it = s_Materials.find(guid);

		if (it != s_Materials.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	Texture* ResourceManager::GetTexture(GUID_Lambda guid)
	{
		auto it = s_Textures.find(guid);

		if (it != s_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	TextureView* ResourceManager::GetTextureView(GUID_Lambda guid)
	{
		auto it = s_TextureViews.find(guid);

		if (it != s_TextureViews.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTextureView called with invalid GUID %u", guid);
		return nullptr;
	}

	Shader* ResourceManager::GetShader(GUID_Lambda guid)
	{
		auto it = s_Shaders.find(guid);

		if (it != s_Shaders.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetShader called with invalid GUID %u", guid);
		return nullptr;
	}

	ISoundEffect3D* ResourceManager::GetSoundEffect(GUID_Lambda guid)
	{
		auto it = s_SoundEffects.find(guid);

		if (it != s_SoundEffects.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetSoundEffect called with invalid GUID %u", guid);
		return nullptr;
	}

	/*
	* Generation of meshlets
	* Reference: https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12MeshShaders
	*/
	struct EdgeEntry
	{
		uint32 i0;
		uint32 i1;
		uint32 i2;

		uint32 Face;
		EdgeEntry* pNext;
	};

	struct InlineMeshlet
	{
		struct PackedTriangle
		{
			uint32 i0 : 10;
			uint32 i1 : 10;
			uint32 i2 : 10;
			uint32 Padding : 2;
		};

		TArray<Mesh::IndexType> UniqueVertexIndices;
		TArray<PackedTriangle> PrimitiveIndices;
	};

	static void GenerateAdjecenyList(Mesh* pMesh, uint32* pAdjecency)
	{
		const uint32 indexCount		= pMesh->IndexCount;
		const uint32 vertexCount	= pMesh->VertexCount;
		const uint32 triangleCount	= (indexCount / 3);

		const Mesh::IndexType* pIndices = pMesh->pIndexArray;
		const Vertex* pVertices = pMesh->pVertexArray;
		
		TArray<Mesh::IndexType> indexList(vertexCount);

		std::unordered_map<size_t, Mesh::IndexType> uniquePositions;
		uniquePositions.reserve(vertexCount);

		std::hash<Vertex> hasher;
		for (uint32 i = 0; i < vertexCount; i++)
		{
			size_t hash = hasher(pMesh->pVertexArray[i]);

			auto it = uniquePositions.find(hash);
			if (it != uniquePositions.end())
			{
				indexList[i] = it->second;
			}
			else
			{
				uniquePositions.insert(std::make_pair(hash, i));
				indexList[i] = i;
			}
		}

		const uint32 hashSize = vertexCount / 3;
		TUniquePtr<EdgeEntry[]> entries(DBG_NEW EdgeEntry[indexCount]);
		TUniquePtr<EdgeEntry*[]> hashTable(DBG_NEW EdgeEntry*[hashSize]);
		ZERO_MEMORY(hashTable.Get(), sizeof(EdgeEntry*) * hashSize);

		uint32 entryIndex = 0;
		for (uint32 face = 0; face < triangleCount; face++)
		{
			uint32 index = face * 3;
			for (uint32 edge = 0; edge < 3; edge++)
			{
				Mesh::IndexType i0 = indexList[pIndices[index + ((edge + 0) % 3)]];
				Mesh::IndexType i1 = indexList[pIndices[index + ((edge + 1) % 3)]];
				Mesh::IndexType i2 = indexList[pIndices[index + ((edge + 2) % 3)]];

				EdgeEntry& entry = entries[entryIndex++];
				entry.i0 = i0;
				entry.i1 = i1;
				entry.i2 = i2;

				uint32 key = entry.i0 % hashSize;
				entry.pNext = hashTable[key];
				entry.Face = face;

				hashTable[key] = &entry;
			}
		}

		memset(pAdjecency, static_cast<uint32>(-1), indexCount * sizeof(uint32));

		for (uint32 face = 0; face < triangleCount; face++)
		{
			uint32 index = face * 3;
			for (uint32 point = 0; point < 3; point++)
			{
				if (pAdjecency[index + point] != static_cast<uint32>(-1))
				{
					continue;
				}

				Mesh::IndexType i0 = indexList[pIndices[index + ((point + 1) % 3)]];
				Mesh::IndexType i1 = indexList[pIndices[index + ((point + 0) % 3)]];
				Mesh::IndexType i2 = indexList[pIndices[index + ((point + 2) % 3)]];

				uint32 key = i0 % hashSize;

				EdgeEntry* pFound = nullptr;
				EdgeEntry* pFoundPrev = nullptr;

				{
					EdgeEntry* pPrev = nullptr;
					for (EdgeEntry* pCurrent = hashTable[key]; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
					{
						if (pCurrent->i1 == i1 && pCurrent->i0 == i0)
						{
							pFound = pCurrent;
							pFoundPrev = pPrev;
							break;
						}
					}
				}

				glm::vec3 n0;
				{
					glm::vec3 p0 = pVertices[i0].Position;
					glm::vec3 p1 = pVertices[i1].Position;
					glm::vec3 p2 = pVertices[i2].Position;
					glm::vec3 e0 = p0 - p1;
					glm::vec3 e1 = p1 - p2;
					n0 = glm::normalize(glm::cross(e0, e1));
				}

				float32 bestDot = -2.0f;
				{
					EdgeEntry* pPrev = pFoundPrev;
					for (EdgeEntry* pCurrent = pFound; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
					{
						glm::vec3 p0 = pVertices[pCurrent->i0].Position;
						glm::vec3 p1 = pVertices[pCurrent->i1].Position;
						glm::vec3 p2 = pVertices[pCurrent->i2].Position;
						glm::vec3 e0 = p0 - p1;
						glm::vec3 e1 = p1 - p2;
						glm::vec3 n1 = glm::normalize(glm::cross(e0, e1));

						float32 dot = glm::dot(e0, e1);
						if (dot > bestDot)
						{
							pFound = pCurrent;
							pFoundPrev = pCurrent;
							bestDot = dot;
						}
					}
				}

				if (pFound)
				{
					if (pFound->Face != static_cast<uint32>(-1))
					{
						if (pFoundPrev != nullptr)
						{
							pFoundPrev->pNext = pFound->pNext;
						}
						else
						{
							hashTable[key] = pFound->pNext;
						}
					}
				}

				pAdjecency[index + point] = pFound->Face;

				uint32 key2 = i1 & hashSize;
				{
					EdgeEntry* pPrev = nullptr;
					for (EdgeEntry* pCurrent = hashTable[key2]; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
					{
						if (pCurrent->Face == face && pCurrent->i0 == i1 && pCurrent->i1 == i0)
						{
							if (pPrev != nullptr)
							{
								pPrev->pNext = pCurrent->pNext;
							}
							else
							{
								hashTable[key2] = pCurrent->pNext;
							}

							break;
						}
					}
				}

				bool linked = false;
				for (uint32 point2 = 0; point2 < point; point2++)
				{
					if (pFound->Face == pAdjecency[index + point2])
					{
						linked = true;
						pAdjecency[index + point] = static_cast<uint32>(-1);
						break;
					}
				}

				if (!linked)
				{
					uint32 edge2 = 0;
					for (; edge2 < 3; edge2++)
					{
						Mesh::IndexType k = pIndices[(pFound->Face * 3) + edge2];
						if (k == static_cast<uint32>(-1))
						{
							continue;
						}

						if (indexList[k] == i0)
						{
							break;
						}
					}

					if (edge2 < 3)
					{
						pAdjecency[pFound->Face * 3 + edge2] = face;
					}
				}
			}
		}
	}

	static bool AddToMeshlet()
	{
	}

	static bool IsMeshletFull()
	{
	}

	static float32 ComputeMeshletScore()
	{
	}

	static glm::vec3 ComputeNormal(glm::vec3 positions[3])
	{
		glm::vec3 e0 = positions[0] - positions[1];
		glm::vec3 e1 = positions[1] - positions[2];
		glm::vec3 normal = glm::normalize(glm::cross(e0, e1));
	}

	static void Meshletize(Mesh* pMesh, TArray<InlineMeshlet> output)
	{
		Mesh::IndexType* pIndices = pMesh->pIndexArray;
		Vertex* pVertices = pMesh->pVertexArray;

		const uint32 vertexCount = pMesh->VertexCount;
		const uint32 indexCount = pMesh->IndexCount;
		const uint32 triangleCount = (indexCount / 3);

		TArray<uint32> adjecenyList(indexCount);
		GenerateAdjecenyList(pMesh, adjecenyList.GetData());

		output.Clear();
		output.EmplaceBack();
		InlineMeshlet* pCurr = &output.GetBack();

		// Using std::vector since it has a bool specialization that stores it in a bitrepresentation
		std::vector<bool> checklist;
		checklist.resize(triangleCount);

		TArray<glm::vec3> positions;
		TArray<glm::vec3> normals;
		TArray<std::pair<uint32, float32>> candidates;
		std::unordered_set<uint32> candidateCheck;
		glm::vec4 sphere;
		glm::vec3 normal;

		uint32 triIndex = 0;
		candidates.EmplaceBack(std::make_pair(triIndex, 0.0f));
		candidateCheck.insert(triIndex);

		while (!candidates.IsEmpty())
		{
			uint32 index = candidates.GetBack().first;
			candidates.PopBack();

			Mesh::IndexType tri[3] =
			{
				pIndices[index * 3 + 0],
				pIndices[index * 3 + 1],
				pIndices[index * 3 + 2],
			};

			VALIDATE(tri[0] < vertexCount);
			VALIDATE(tri[1] < vertexCount);
			VALIDATE(tri[2] < vertexCount);

			if (AddToMeshlet())
			{
				checklist[index] = true;

				glm::vec3 points[3] =
				{
					pVertices[tri[0]].Position,
					pVertices[tri[1]].Position,
					pVertices[tri[2]].Position,
				};

				positions.PushBack(points[0]);
				positions.PushBack(points[1]);
				positions.PushBack(points[2]);

				normal = ComputeNormal(points);
				normals.PushBack(normal);

				const uint32 adjIndex = index * 3;
				uint32 adj[3] =
				{
					adjecenyList[adjIndex + 0],
					adjecenyList[adjIndex + 1],
					adjecenyList[adjIndex + 2],
				};

				for (uint32 i = 0; i < 3; i++)
				{
					if (adj[i] == static_cast<uint32>(-1))
					{
						continue;
					}

					if (checklist[adj[i]])
					{
						continue;
					}

					if (candidateCheck.count(adj[i]))
					{
						continue;
					}

					candidates.PushBack(std::make_pair(adj[i], FLT_MAX));
					candidateCheck.insert(adj[i]);
				}

				for (uint32 i = 0; i < static_cast<uint32>(candidates.GetSize()); i++)
				{
					uint32 candidate = candidates[i].first;
					Mesh::IndexType triIndices[3] =
					{
						pIndices[(candidate * 3) + 0],
						pIndices[(candidate * 3) + 1],
						pIndices[(candidate * 3) + 2],
					};

					VALIDATE(triIndices[0] < vertexCount);
					VALIDATE(triIndices[1] < vertexCount);
					VALIDATE(triIndices[2] < vertexCount);

					glm::vec3 triVerts[3] =
					{
						positions[triIndices[0]],
						positions[triIndices[1]],
						positions[triIndices[2]],
					};

					candidates[i].second = ComputeMeshletScore(*pCurr, sphere, normal, triIndices, triVerts);
				}

				if (IsMeshletFull())
				{
					positions.Clear();
					normals.Clear();
					
					candidateCheck.clear();

					if (!candidates.IsEmpty())
					{
						candidates[0] = candidates.GetBack();
						candidates.Resize(1);
						candidateCheck.insert(candidates[0].first);
					}

					output.EmplaceBack();
					pCurr = &output.GetBack();
				}
				else
				{
					std::sort(candidates.begin(), candidates.end(), );
				}
			}
			else
			{
				if (candidates.IsEmpty())
				{
					positions.Clear();
					normals.Clear();

					candidateCheck.clear();

					output.EmplaceBack();
					pCurr = &output.GetBack();
				}
			}

			if (candidates.IsEmpty())
			{
				while (triIndex < triangleCount && checklist[triIndex])
				{
					triIndex++;
				}

				if (triIndex == triangleCount)
				{
					break;
				}

				candidates.PushBack(std::make_pair(triIndex, 0.0f));
				candidateCheck.insert(triIndex);
			}
		}
	}

	void ResourceManager::GenerateMeshlets(Mesh* pMesh)
	{
		VALIDATE(pMesh->pIndexArray		!= nullptr);
		VALIDATE(pMesh->pVertexArray	!= nullptr);

		TArray<InlineMeshlet> builtMeshlets;
		Meshletize(pMesh, builtMeshlets);
	}

	GUID_Lambda ResourceManager::RegisterLoadedMesh(const String& name, Mesh* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedResource = nullptr;

		//Spinlock
		{
			guid						= s_NextFreeGUID++;
			ppMappedResource			= &s_Meshes[guid]; //Creates new entry if not existing
			s_MeshNamesToGUIDs[name]	= guid;
		}

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedMaterial(const String& name, Material* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Material** ppMappedResource = nullptr;

		//Spinlock
		{
			guid							= s_NextFreeGUID++;
			ppMappedResource				= &s_Materials[guid]; //Creates new entry if not existing
			s_MaterialNamesToGUIDs[name]	= guid;
		}

		pResource->pAlbedoMap					= pResource->pAlbedoMap					!= nullptr ? pResource->pAlbedoMap					: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pNormalMap					= pResource->pNormalMap					!= nullptr ? pResource->pNormalMap					: s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMap			= pResource->pAmbientOcclusionMap		!= nullptr ? pResource->pAmbientOcclusionMap		: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pMetallicMap					= pResource->pMetallicMap				!= nullptr ? pResource->pMetallicMap				: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pRoughnessMap				= pResource->pRoughnessMap				!= nullptr ? pResource->pRoughnessMap				: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		
		pResource->pAlbedoMapView				= pResource->pAlbedoMapView				!= nullptr ? pResource->pAlbedoMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pNormalMapView				= pResource->pNormalMapView				!= nullptr ? pResource->pNormalMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMapView		= pResource->pAmbientOcclusionMapView	!= nullptr ? pResource->pAmbientOcclusionMapView	: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pMetallicMapView				= pResource->pMetallicMapView			!= nullptr ? pResource->pMetallicMapView			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pRoughnessMapView			= pResource->pRoughnessMapView			!= nullptr ? pResource->pRoughnessMapView			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedTexture(Texture* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Texture** ppMappedTexture = nullptr;
		TextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid				= s_NextFreeGUID++;
			ppMappedTexture		= &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView	= &s_TextureViews[guid]; //Creates new entry if not existing
			s_TextureNamesToGUIDs[pResource->GetDesc().DebugName]	= guid;
		}

		(*ppMappedTexture) = pResource;

		ASSERT(pResource != nullptr);
		
		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= pResource->GetDesc().DebugName + " Texture View";
		textureViewDesc.pTexture		= pResource;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= pResource->GetDesc().Format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= pResource->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pResource->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name)
	{
		auto guidIt = namesToGUIDs.find(name);

		if (guidIt != namesToGUIDs.end())
			return guidIt->second;

		if (name.length() > 0)
			LOG_ERROR("[ResourceManager]: Resource \"%s\" could not be fouund in ResourceManager", name.c_str());

		return GUID_NONE;
	}

	void ResourceManager::InitDefaultResources()
	{
		s_Meshes[GUID_NONE]					= nullptr;
		s_Materials[GUID_NONE]				= nullptr;
		s_Textures[GUID_NONE]				= nullptr;
		s_TextureViews[GUID_NONE]			= nullptr;
		s_Shaders[GUID_NONE]				= nullptr;
		s_SoundEffects[GUID_NONE]			= nullptr;

		{
			s_MeshNamesToGUIDs["Quad"]			= GUID_MESH_QUAD;
			s_Meshes[GUID_MESH_QUAD]			= MeshFactory::CreateQuad();
		}

		{
			byte defaultColor[4]				= { 255, 255, 255, 255 };
			byte defaultNormal[4]				= { 127, 127, 255, 0   };
			void* pDefaultColor					= (void*)defaultColor;
			void* pDefaultNormal				= (void*)defaultNormal;
			Texture* pDefaultColorMap			= ResourceLoader::LoadTextureArrayFromMemory("Default Color Map", &pDefaultColor, 1, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, false);
			Texture* pDefaultNormalMap			= ResourceLoader::LoadTextureArrayFromMemory("Default Normal Map", &pDefaultNormal, 1, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, false);

			s_TextureNamesToGUIDs[pDefaultColorMap->GetDesc().DebugName]		= GUID_TEXTURE_DEFAULT_COLOR_MAP;
			s_TextureNamesToGUIDs[pDefaultNormalMap->GetDesc().DebugName]	= GUID_TEXTURE_DEFAULT_NORMAL_MAP;
			s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP]		= pDefaultColorMap;
			s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP]		= pDefaultNormalMap;

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

			s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP]		= RenderSystem::GetDevice()->CreateTextureView(&defaultColorMapViewDesc);
			s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP]		= RenderSystem::GetDevice()->CreateTextureView(&defaultNormalMapViewDesc);
		}

		{
			Material* pDefaultMaterial = DBG_NEW Material();
			pDefaultMaterial->pAlbedoMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMap				= s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAmbientOcclusionMap		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pMetallicMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pRoughnessMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			pDefaultMaterial->pAlbedoMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAmbientOcclusionMapView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pMetallicMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pRoughnessMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			s_MaterialNamesToGUIDs["Default Material"]	= GUID_MATERIAL_DEFAULT;
			s_Materials[GUID_MATERIAL_DEFAULT] = pDefaultMaterial;
		}

		{
			Material* pDefaultEmissiveMaterial = DBG_NEW Material();
			pDefaultEmissiveMaterial->pAlbedoMap					= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pNormalMap					= s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultEmissiveMaterial->pAmbientOcclusionMap			= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pMetallicMap					= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pRoughnessMap					= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			pDefaultEmissiveMaterial->pAlbedoMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pNormalMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultEmissiveMaterial->pAmbientOcclusionMapView		= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pMetallicMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultEmissiveMaterial->pRoughnessMapView				= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			pDefaultEmissiveMaterial->Properties.EmissionStrength	= DEFAULT_EMISSIVE_EMISSION_STRENGTH;

			s_MaterialNamesToGUIDs["Default Emissive Material"] = GUID_MATERIAL_DEFAULT_EMISSIVE;
			s_Materials[GUID_MATERIAL_DEFAULT_EMISSIVE] = pDefaultEmissiveMaterial;
		}
	}
}
