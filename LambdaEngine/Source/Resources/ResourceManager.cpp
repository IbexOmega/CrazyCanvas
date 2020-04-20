#include "Resources/ResourceManager.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Log/Log.h"

#include <utility>

#define SAFEDELETE_ALL(map) for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); } map.clear();
#define SAFERELEASE_ALL(map) for (auto it = map.begin(); it != map.end(); it++) { SAFERELEASE(it->second); } map.clear();

namespace LambdaEngine
{
	GUID_Lambda ResourceManager::s_NextFreeGUID = SMALLEST_UNRESERVED_GUID;

	ResourceManager::ResourceManager(IGraphicsDevice* pGraphicsDevice, IAudioDevice* pAudioDevice) :
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
		SAFERELEASE_ALL(m_TextureViews);
		SAFERELEASE_ALL(m_Shaders);
		SAFEDELETE_ALL(m_SoundEffects);
	}

	bool ResourceManager::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GameObject>& result)
	{
		std::vector<GameObject> sceneLocalGameObjects;
		std::vector<Mesh*> meshes;
		std::vector<Material*> materials;
		std::vector<ITexture*> textures;

		if (!ResourceLoader::LoadSceneFromFile(pDir, pFilename, sceneLocalGameObjects, meshes, materials, textures))
		{
			return false;
		}

		result = std::vector<GameObject>(sceneLocalGameObjects.begin(), sceneLocalGameObjects.end());

		for (uint32 i = 0; i < textures.size(); i++)
		{
			ITexture* pTexture = textures[i];

			GUID_Lambda guid = RegisterLoadedTexture(pTexture);

			//RegisterLoadedTexture will create a TextureView for the texture, this needs to be registered in the correct materials
			for (uint32 i = 0; i < materials.size(); i++)
			{
				Material* pMaterial = materials[i];

				if (pMaterial->pAlbedoMap == pTexture)
					pMaterial->pAlbedoMapView = m_TextureViews[guid];

				if (pMaterial->pNormalMap == pTexture)
					pMaterial->pNormalMapView = m_TextureViews[guid];

				if (pMaterial->pAmbientOcclusionMap == pTexture)
					pMaterial->pAmbientOcclusionMapView = m_TextureViews[guid];

				if (pMaterial->pMetallicMap == pTexture)
					pMaterial->pMetallicMapView = m_TextureViews[guid];

				if (pMaterial->pRoughnessMap == pTexture)
					pMaterial->pRoughnessMapView = m_TextureViews[guid];
			}
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

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromFile(pFilepath);

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

		(*ppMappedMesh) = ResourceLoader::LoadMeshFromMemory(pVertices, numVertices, pIndices, numIndices);

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

		ITexture* pAlbedoMap						= albedoMap				!= GUID_NONE ? m_Textures[albedoMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pNormalMap						= normalMap				!= GUID_NONE ? m_Textures[normalMap]			: m_Textures[DEFAULT_NORMAL_MAP];
		ITexture* pAmbientOcclusionMap				= ambientOcclusionMap	!= GUID_NONE ? m_Textures[ambientOcclusionMap]	: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pMetallicMap						= metallicMap			!= GUID_NONE ? m_Textures[metallicMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pRoughnessMap						= roughnessMap			!= GUID_NONE ? m_Textures[roughnessMap]			: m_Textures[DEFAULT_COLOR_MAP];

		ITextureView* pAlbedoMapView				= albedoMap				!= GUID_NONE ? m_TextureViews[albedoMap]			: m_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pNormalMapView				= normalMap				!= GUID_NONE ? m_TextureViews[normalMap]			: m_TextureViews[DEFAULT_NORMAL_MAP];
		ITextureView* pAmbientOcclusionMapView		= ambientOcclusionMap	!= GUID_NONE ? m_TextureViews[ambientOcclusionMap]	: m_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pMetallicMapView				= metallicMap			!= GUID_NONE ? m_TextureViews[metallicMap]			: m_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pRoughnessMapView				= roughnessMap			!= GUID_NONE ? m_TextureViews[roughnessMap]			: m_TextureViews[DEFAULT_COLOR_MAP];
		
		pMappedMaterial->Properties					= properties;

		pMappedMaterial->pAlbedoMap					= pAlbedoMap;
		pMappedMaterial->pNormalMap					= pNormalMap;
		pMappedMaterial->pAmbientOcclusionMap		= pAmbientOcclusionMap;
		pMappedMaterial->pMetallicMap				= pMetallicMap;
		pMappedMaterial->pRoughnessMap				= pRoughnessMap;

		pMappedMaterial->pAlbedoMapView				= pAlbedoMapView;
		pMappedMaterial->pNormalMapView				= pNormalMapView;
		pMappedMaterial->pAmbientOcclusionMapView	= pAmbientOcclusionMapView;
		pMappedMaterial->pMetallicMapView			= pMetallicMapView;
		pMappedMaterial->pRoughnessMapView			= pRoughnessMapView;
		
		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureFromFile(const char* pFilepath, EFormat format, bool generateMips)
	{
		GUID_Lambda guid = GUID_NONE;
		ITexture** ppMappedTexture = nullptr;
		ITextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &m_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &m_TextureViews[guid]; //Creates new entry if not existing
		}

		ITexture* pTexture = ResourceLoader::LoadTextureFromFile(pFilepath, format, generateMips);

		(*ppMappedTexture) = pTexture;

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.pName			= "Resource Manager Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.MiplevelCount	= pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = m_pGraphicsDevice->CreateTextureView(textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadTextureFromMemory(const char* pName, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		GUID_Lambda guid = GUID_NONE;
		ITexture** ppMappedTexture = nullptr;
		ITextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &m_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &m_TextureViews[guid]; //Creates new entry if not existing
		}

		ITexture* pTexture = ResourceLoader::LoadTextureFromMemory(pName, pData, width, height, format, usageFlags, generateMips);

		(*ppMappedTexture) = pTexture;

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.pName			= "Resource Manager Texture View";
		textureViewDesc.pTexture		= pTexture;
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.MiplevelCount	= pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = m_pGraphicsDevice->CreateTextureView(textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadShaderFromFile(const char* pFilepath, FShaderStageFlags stage, EShaderLang lang, ShaderConstant* pConstants, uint32 shaderConstantCount, const char* pEntryPoint)
	{
		GUID_Lambda guid = GUID_NONE;
		IShader** ppMappedShader = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedShader = &m_Shaders[guid]; //Creates new entry if not existing
		}

		(*ppMappedShader) = ResourceLoader::LoadShaderFromFile(pFilepath, stage, lang, pConstants, shaderConstantCount, pEntryPoint);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadSoundEffectFromFile(const char* pFilepath)
	{
		GUID_Lambda guid = GUID_NONE;
		ISoundEffect3D** ppMappedSoundEffect = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedSoundEffect = &m_SoundEffects[guid]; //Creates new entry if not existing
		}

		(*ppMappedSoundEffect) = ResourceLoader::LoadSoundEffectFromFile(pFilepath);

		return guid;
	}

	Mesh* ResourceManager::GetMesh(GUID_Lambda guid)
	{
		auto it = m_Meshes.find(guid);

		if (it != m_Meshes.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	const Mesh* ResourceManager::GetMesh(GUID_Lambda guid) const
	{
		auto it = m_Meshes.find(guid);

		if (it != m_Meshes.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMesh called with invalid GUID %u", guid);
		return nullptr;
	}

	Material* ResourceManager::GetMaterial(GUID_Lambda guid)
	{
		auto it = m_Materials.find(guid);

		if (it != m_Materials.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	const Material* ResourceManager::GetMaterial(GUID_Lambda guid) const
	{
		auto it = m_Materials.find(guid);

		if (it != m_Materials.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetMaterial called with invalid GUID %u", guid);
		return nullptr;
	}

	ITexture* ResourceManager::GetTexture(GUID_Lambda guid)
	{
		auto it = m_Textures.find(guid);

		if (it != m_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	const ITexture* ResourceManager::GetTexture(GUID_Lambda guid) const
	{
		auto it = m_Textures.find(guid);

		if (it != m_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	ITextureView* ResourceManager::GetTextureView(GUID_Lambda guid)
	{
		auto it = m_TextureViews.find(guid);

		if (it != m_TextureViews.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTextureView called with invalid GUID %u", guid);
		return nullptr;
	}

	const ITextureView* ResourceManager::GetTextureView(GUID_Lambda guid) const
	{
		auto it = m_TextureViews.find(guid);

		if (it != m_TextureViews.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTextureView called with invalid GUID %u", guid);
		return nullptr;
	}

	IShader* ResourceManager::GetShader(GUID_Lambda guid)
	{
		auto it = m_Shaders.find(guid);

		if (it != m_Shaders.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetShader called with invalid GUID %u", guid);
		return nullptr;
	}

	const IShader* ResourceManager::GetShader(GUID_Lambda guid) const
	{
		auto it = m_Shaders.find(guid);

		if (it != m_Shaders.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetShader called with invalid GUID %u", guid);
		return nullptr;
	}

	ISoundEffect3D* ResourceManager::GetSoundEffect(GUID_Lambda guid)
	{
		auto it = m_SoundEffects.find(guid);

		if (it != m_SoundEffects.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetSoundEffect called with invalid GUID %u", guid);
		return nullptr;
	}

	const ISoundEffect3D* ResourceManager::GetSoundEffect(GUID_Lambda guid) const
	{
		auto it = m_SoundEffects.find(guid);

		if (it != m_SoundEffects.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetSoundEffect called with invalid GUID %u", guid);
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

		pResource->pAlbedoMap					= pResource->pAlbedoMap					!= nullptr ? pResource->pAlbedoMap					: m_Textures[DEFAULT_COLOR_MAP];
		pResource->pNormalMap					= pResource->pNormalMap					!= nullptr ? pResource->pNormalMap					: m_Textures[DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMap			= pResource->pAmbientOcclusionMap		!= nullptr ? pResource->pAmbientOcclusionMap		: m_Textures[DEFAULT_COLOR_MAP];
		pResource->pMetallicMap					= pResource->pMetallicMap				!= nullptr ? pResource->pMetallicMap				: m_Textures[DEFAULT_COLOR_MAP];
		pResource->pRoughnessMap				= pResource->pRoughnessMap				!= nullptr ? pResource->pRoughnessMap				: m_Textures[DEFAULT_COLOR_MAP];
		
		pResource->pAlbedoMapView				= pResource->pAlbedoMapView				!= nullptr ? pResource->pAlbedoMapView				: m_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pNormalMapView				= pResource->pNormalMapView				!= nullptr ? pResource->pNormalMapView				: m_TextureViews[DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMapView		= pResource->pAmbientOcclusionMapView	!= nullptr ? pResource->pAmbientOcclusionMapView	: m_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pMetallicMapView				= pResource->pMetallicMapView			!= nullptr ? pResource->pMetallicMapView			: m_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pRoughnessMapView			= pResource->pRoughnessMapView			!= nullptr ? pResource->pRoughnessMapView			: m_TextureViews[DEFAULT_COLOR_MAP];

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedTexture(ITexture* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		ITexture** ppMappedTexture = nullptr;
		ITextureView** ppMappedTextureView = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedTexture = &m_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &m_TextureViews[guid]; //Creates new entry if not existing
		}

		(*ppMappedTexture) = pResource;

        ASSERT(pResource != nullptr);
        
		TextureViewDesc textureViewDesc = {};
		textureViewDesc.pName			= "Resource Manager Texture View";
		textureViewDesc.pTexture		= pResource;
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= pResource->GetDesc().Format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.MiplevelCount	= pResource->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= pResource->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		(*ppMappedTextureView) = m_pGraphicsDevice->CreateTextureView(textureViewDesc);

		return guid;
	}

	void ResourceManager::InitDefaultResources()
	{
		m_Meshes[GUID_NONE]					= nullptr;
		m_Materials[GUID_NONE]				= nullptr;
		m_Textures[GUID_NONE]				= nullptr;

		byte defaultColor[4]				= { 255, 255, 255, 255 };
		byte defaultNormal[4]				= { 127, 127, 127, 0   };
		ITexture* pDefaultColorMap			= ResourceLoader::LoadTextureFromMemory("Default Color Map", defaultColor, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, false);
		ITexture* pDefaultNormalMap			= ResourceLoader::LoadTextureFromMemory("Default Normal Map", defaultNormal, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, false);

		m_Textures[DEFAULT_COLOR_MAP]		= pDefaultColorMap;
		m_Textures[DEFAULT_NORMAL_MAP]		= pDefaultNormalMap;

		TextureViewDesc defaultColorMapViewDesc = {};
		defaultColorMapViewDesc.pName			= "Default Color Map View";
		defaultColorMapViewDesc.pTexture		= pDefaultColorMap;
		defaultColorMapViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		defaultColorMapViewDesc.Format			= pDefaultColorMap->GetDesc().Format;
		defaultColorMapViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		defaultColorMapViewDesc.MiplevelCount	= pDefaultColorMap->GetDesc().Miplevels;
		defaultColorMapViewDesc.ArrayCount		= pDefaultColorMap->GetDesc().ArrayCount;
		defaultColorMapViewDesc.Miplevel		= 0;
		defaultColorMapViewDesc.ArrayIndex		= 0;

		TextureViewDesc defaultNormalMapViewDesc = {};
		defaultNormalMapViewDesc.pName			= "Default Normal Map View";
		defaultNormalMapViewDesc.pTexture		= pDefaultNormalMap;
		defaultNormalMapViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		defaultNormalMapViewDesc.Format			= pDefaultNormalMap->GetDesc().Format;
		defaultNormalMapViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		defaultNormalMapViewDesc.MiplevelCount	= pDefaultNormalMap->GetDesc().Miplevels;
		defaultNormalMapViewDesc.ArrayCount		= pDefaultNormalMap->GetDesc().ArrayCount;
		defaultNormalMapViewDesc.Miplevel		= 0;
		defaultNormalMapViewDesc.ArrayIndex		= 0;

		m_TextureViews[DEFAULT_COLOR_MAP]		= m_pGraphicsDevice->CreateTextureView(defaultColorMapViewDesc);
		m_TextureViews[DEFAULT_NORMAL_MAP]		= m_pGraphicsDevice->CreateTextureView(defaultNormalMapViewDesc);

		Material* pDefaultMaterial = DBG_NEW Material();
		pDefaultMaterial->pAlbedoMap				= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pNormalMap				= m_Textures[DEFAULT_NORMAL_MAP];
		pDefaultMaterial->pAmbientOcclusionMap		= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pMetallicMap				= m_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pRoughnessMap				= m_Textures[DEFAULT_COLOR_MAP];

		pDefaultMaterial->pAlbedoMapView			= m_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pNormalMapView			= m_TextureViews[DEFAULT_NORMAL_MAP];
		pDefaultMaterial->pAmbientOcclusionMapView	= m_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pMetallicMapView			= m_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pRoughnessMapView			= m_TextureViews[DEFAULT_COLOR_MAP];

		m_Materials[DEFAULT_MATERIAL] = pDefaultMaterial;

	}
}
