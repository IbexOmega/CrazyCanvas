#include "Resources/ResourceManager.h"

#include "Log/Log.h"

#include <utility>

#define SAFEDELETE_ALL(map) for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); } map.clear();
#define SAFERELEASE_ALL(map) for (auto it = map.begin(); it != map.end(); it++) { SAFERELEASE(it->second); } map.clear();

namespace LambdaEngine
{
	GUID_Lambda ResourceManager::s_NextFreeGUID = ResourceManager::SMALLEST_UNRESERVED_GUID;

	ResourceManager::ResourceManager(IGraphicsDevice* pGraphicsDevice, AudioDevice* pAudioDevice) :
		m_pGraphicsDevice(pGraphicsDevice),
		m_pAudioDevice(pAudioDevice)
	{
		InitDefaultResources();
	}

	ResourceManager::~ResourceManager()
	{
		SAFEDELETE_ALL(m_Meshes);
		SAFEDELETE_ALL(m_Materials);
		SAFERELEASE_ALL(m_Textures);
		SAFEDELETE_ALL(m_Sounds);
	}

	bool ResourceManager::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GameObject>& result)
	{
		std::vector<GameObject> sceneLocalGameObjects;
		std::vector<Mesh*> meshes;
		std::vector<Material*> materials;
		std::vector<ITexture*> textures;

		if (!ResourceLoader::LoadSceneFromFile(m_pGraphicsDevice, pDir, pFilename, sceneLocalGameObjects, meshes, materials, textures))
		{
			return false;
		}

		result = std::vector<GameObject>(sceneLocalGameObjects.begin(), sceneLocalGameObjects.end());

		for (uint32 i = 0; i < textures.size(); i++)
		{
			RegisterLoadedTexture(textures[i]);
		}

		for (uint32 i = 0; i < meshes.size(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMesh(meshes[i]);

			for (uint32 g = 0; g < sceneLocalGameObjects.size(); g++)
			{
				if (sceneLocalGameObjects[g].Mesh == i)
				{
					result[g].Mesh = guid;
				}
			}
		}

		for (uint32 i = 0; i < materials.size(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMaterial(materials[i]);

			for (uint32 g = 0; g < sceneLocalGameObjects.size(); g++)
			{
				if (sceneLocalGameObjects[g].Material == i)
				{
					result[g].Material = guid;
				}
			}
		}

		for (uint32 g = 0; g < sceneLocalGameObjects.size(); g++)
		{
			if (sceneLocalGameObjects[g].Mesh >= meshes.size())
			{
				LOG_ERROR("[ResourceManager]: GameObject %u in Scene %s has no Mesh", g, pFilename);
			}

			if (sceneLocalGameObjects[g].Material >= materials.size())
			{
				result[g].Material = DEFAULT_MATERIAL;
				LOG_WARNING("[ResourceManager]: GameObject %u in Scene %s has no Material, default Material assigned", g, pFilename);
			}
		}

		return true;
	}

	GUID_Lambda ResourceManager::LoadMeshFromFile(const char* pFilepath)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedMesh = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedMesh = &m_Meshes[guid]; //Creates new entry if not existing
		}

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromFile(m_pGraphicsDevice, pFilepath);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedMesh = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedMesh = &m_Meshes[guid]; //Creates new entry if not existing
		}

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromMemory(m_pGraphicsDevice, pVertices, numVertices, pIndices, numIndices);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties)
	{
		GUID_Lambda guid = GUID_NONE;
		Material* pMappedMaterial = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			pMappedMaterial = m_Materials[guid]; //Creates new entry if not existing
		}

		ITexture* pAlbedoMap				= albedoMap				!= GUID_NONE ? m_Textures[albedoMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pNormalMap				= normalMap				!= GUID_NONE ? m_Textures[normalMap]			: m_Textures[DEFAULT_NORMAL_MAP];
		ITexture* pAmbientOcclusionMap		= ambientOcclusionMap	!= GUID_NONE ? m_Textures[ambientOcclusionMap]	: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pMetallicMap				= metallicMap			!= GUID_NONE ? m_Textures[metallicMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pRoughnessMap				= roughnessMap			!= GUID_NONE ? m_Textures[roughnessMap]			: m_Textures[DEFAULT_COLOR_MAP];
		
		pMappedMaterial->pAlbedoMap					= pAlbedoMap;
		pMappedMaterial->pNormalMap					= pNormalMap;
		pMappedMaterial->pAmbientOcclusionMap		= pAmbientOcclusionMap;
		pMappedMaterial->pMetallicMap				= pMetallicMap;
		pMappedMaterial->pRoughnessMap				= pRoughnessMap;
		pMappedMaterial->Properties					= properties;
		
		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureFromFile(const char* pFilepath)
	{
		UNREFERENCED_VARIABLE(pFilepath);

		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromFile");
		return GUID_NONE;
	}

	GUID_Lambda ResourceManager::LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		UNREFERENCED_VARIABLE(pData);
		UNREFERENCED_VARIABLE(width);
		UNREFERENCED_VARIABLE(height);
		UNREFERENCED_VARIABLE(usageFlags);
		UNREFERENCED_VARIABLE(format);
		UNREFERENCED_VARIABLE(generateMips);

		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromMemory");
		return GUID_NONE;
	}

	GUID_Lambda ResourceManager::LoadSoundFromFile(const char* pFilepath)
	{
		GUID_Lambda guid = GUID_NONE;
		SoundEffect3D** ppMappedSound = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedSound = &m_Sounds[guid]; //Creates new entry if not existing
		}

		(*ppMappedSound) = ResourceLoader::LoadSoundFromFile(m_pAudioDevice, pFilepath);

		return guid;
	}

	Mesh* ResourceManager::GetMesh(GUID_Lambda guid)
	{
		auto& it = m_Meshes.find(guid);

		if (it != m_Meshes.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	const Mesh* ResourceManager::GetMesh(GUID_Lambda guid) const
	{
		auto& it = m_Meshes.find(guid);

		if (it != m_Meshes.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	Material* ResourceManager::GetMaterial(GUID_Lambda guid)
	{
		auto& it = m_Materials.find(guid);

		if (it != m_Materials.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	const Material* ResourceManager::GetMaterial(GUID_Lambda guid) const
	{
		auto& it = m_Materials.find(guid);

		if (it != m_Materials.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	ITexture* ResourceManager::GetTexture(GUID_Lambda guid)
	{
		auto& it = m_Textures.find(guid);

		if (it != m_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	const ITexture* ResourceManager::GetTexture(GUID_Lambda guid) const
	{
		auto& it = m_Textures.find(guid);

		if (it != m_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	SoundEffect3D* ResourceManager::GetSound(GUID_Lambda guid)
	{
		auto& it = m_Sounds.find(guid);

		if (it != m_Sounds.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetSound called with invalid GUID %u", guid);
		return nullptr;
	}

	const SoundEffect3D* ResourceManager::GetSound(GUID_Lambda guid) const
	{
		auto& it = m_Sounds.find(guid);

		if (it != m_Sounds.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetSound called with invalid GUID %u", guid);
		return nullptr;
	}

	GUID_Lambda ResourceManager::RegisterLoadedMesh(Mesh* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedResource = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedResource = &m_Meshes[guid]; //Creates new entry if not existing
		}

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedMaterial(Material* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Material** ppMappedResource = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedResource = &m_Materials[guid]; //Creates new entry if not existing
		}

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedTexture(ITexture* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		ITexture** ppMappedResource = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedResource = &m_Textures[guid]; //Creates new entry if not existing
		}

		(*ppMappedResource) = pResource;

		return guid;
	}

	void ResourceManager::InitDefaultResources()
	{
		m_Meshes[GUID_NONE]		= nullptr;
		m_Materials[GUID_NONE]	= nullptr;
		m_Textures[GUID_NONE]	= nullptr;

		m_Textures[DEFAULT_COLOR_MAP] = nullptr; //Implement
		m_Textures[DEFAULT_NORMAL_MAP] = nullptr; //Implement

		Material* pDefaultMaterial = DBG_NEW Material();
		pDefaultMaterial->pAlbedoMap				= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pNormalMap				= m_Textures[DEFAULT_NORMAL_MAP];
		pDefaultMaterial->pAmbientOcclusionMap		= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pMetallicMap				= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pRoughnessMap				= m_Textures[DEFAULT_COLOR_MAP];

		m_Materials[DEFAULT_MATERIAL] = pDefaultMaterial;

	}
}