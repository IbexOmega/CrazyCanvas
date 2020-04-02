#include "Resources/ResourceManager.h"

#include "Log/Log.h"

#include <utility>

#define SAFEDELETE_ALL(map) for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); } map.clear();

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
		SAFEDELETE_ALL(m_Textures);
		SAFEDELETE_ALL(m_Sounds);
	}

	bool ResourceManager::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& result)
	{
		std::vector<Mesh*> meshes;
		std::vector<Material*> materials;
		std::vector<ITexture*> textures;

		if (!ResourceLoader::LoadSceneFromFile(m_pGraphicsDevice, pDir, pFilename, result, meshes, materials, textures))
		{
			return false;
		}

		for (uint32 i = 0; i < textures.size(); i++)
		{
			RegisterLoadedTexture(textures[i]);
		}

		for (uint32 i = 0; i < meshes.size(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMesh(meshes[i]);

			for (GraphicsObject graphicsObject : result)
			{
				if (graphicsObject.Mesh == i) graphicsObject.Mesh = guid;
			}
		}

		for (uint32 i = 0; i < materials.size(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMaterial(materials[i]);

			for (GraphicsObject graphicsObject : result)
			{
				if (graphicsObject.Material == i) graphicsObject.Material = guid;
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

	GUID_Lambda ResourceManager::LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambienOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties)
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
		ITexture* pAmbientOcclusionMap		= ambienOcclusionMap	!= GUID_NONE ? m_Textures[ambienOcclusionMap]	: m_Textures[DEFAULT_COLOR_MAP];
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
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromFile");
		return GUID_NONE;
	}

	GUID_Lambda ResourceManager::LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromMemory");
		return GUID_NONE;
	}

	GUID_Lambda ResourceManager::LoadSoundFromFile(const char* pFilepath, ESoundFlags flags)
	{
		GUID_Lambda guid = GUID_NONE;
		Sound** ppMappedSound = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedSound = &m_Sounds[guid]; //Creates new entry if not existing
		}

		(*ppMappedSound) = ResourceLoader::LoadSoundFromFile(m_pAudioDevice, pFilepath, flags);

		return guid;
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
	}
}