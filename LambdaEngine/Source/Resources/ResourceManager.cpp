#include "Resources/ResourceManager.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Log/Log.h"

#include "Rendering/RenderSystem.h"

#include <utility>

#define SAFEDELETE_ALL(map)     for (auto it = map.begin(); it != map.end(); it++) { SAFEDELETE(it->second); } map.clear()
#define SAFERELEASE_ALL(map)    for (auto it = map.begin(); it != map.end(); it++) { SAFERELEASE(it->second); } map.clear()

namespace LambdaEngine
{
	GUID_Lambda												ResourceManager::s_NextFreeGUID = SMALLEST_UNRESERVED_GUID;

	std::unordered_map<GUID_Lambda, Mesh*>					ResourceManager::s_Meshes;
	std::unordered_map<GUID_Lambda, Material*>				ResourceManager::s_Materials;
	std::unordered_map<GUID_Lambda, ITexture*>				ResourceManager::s_Textures;
	std::unordered_map<GUID_Lambda, ITextureView*>			ResourceManager::s_TextureViews;
	std::unordered_map<GUID_Lambda, IShader*>				ResourceManager::s_Shaders;
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
			ppMappedMesh = &s_Meshes[guid]; //Creates new entry if not existing
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
			ppMappedMesh = &s_Meshes[guid]; //Creates new entry if not existing
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
			pMappedMaterial = s_Materials[guid]; //Creates new entry if not existing
		}

		ITexture* pAlbedoMap						= albedoMap				!= GUID_NONE ? s_Textures[albedoMap]			: s_Textures[DEFAULT_COLOR_MAP];
		ITexture* pNormalMap						= normalMap				!= GUID_NONE ? s_Textures[normalMap]			: s_Textures[DEFAULT_NORMAL_MAP];
		ITexture* pAmbientOcclusionMap				= ambientOcclusionMap	!= GUID_NONE ? s_Textures[ambientOcclusionMap]	: s_Textures[DEFAULT_COLOR_MAP];
		ITexture* pMetallicMap						= metallicMap			!= GUID_NONE ? s_Textures[metallicMap]			: s_Textures[DEFAULT_COLOR_MAP];
		ITexture* pRoughnessMap						= roughnessMap			!= GUID_NONE ? s_Textures[roughnessMap]			: s_Textures[DEFAULT_COLOR_MAP];

		ITextureView* pAlbedoMapView				= albedoMap				!= GUID_NONE ? s_TextureViews[albedoMap]			: s_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pNormalMapView				= normalMap				!= GUID_NONE ? s_TextureViews[normalMap]			: s_TextureViews[DEFAULT_NORMAL_MAP];
		ITextureView* pAmbientOcclusionMapView		= ambientOcclusionMap	!= GUID_NONE ? s_TextureViews[ambientOcclusionMap]	: s_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pMetallicMapView				= metallicMap			!= GUID_NONE ? s_TextureViews[metallicMap]			: s_TextureViews[DEFAULT_COLOR_MAP];
		ITextureView* pRoughnessMapView				= roughnessMap			!= GUID_NONE ? s_TextureViews[roughnessMap]			: s_TextureViews[DEFAULT_COLOR_MAP];
		
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
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
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

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

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
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
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

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadShaderFromFile(const char* pFilepath, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint)
	{
		GUID_Lambda guid = GUID_NONE;
		IShader** ppMappedShader = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedShader = &s_Shaders[guid]; //Creates new entry if not existing
		}

		ShaderLoadDesc loadDesc = {};
		loadDesc.pFilepath				= pFilepath;
		loadDesc.Stage					= stage;
		loadDesc.Lang					= lang;
		loadDesc.pEntryPoint			= pEntryPoint;

		s_ShaderLoadConfigurations[guid] = loadDesc;

		(*ppMappedShader) = ResourceLoader::LoadShaderFromFile(pFilepath, stage, lang, pEntryPoint);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadSoundEffectFromFile(const char* pFilepath)
	{
		GUID_Lambda guid = GUID_NONE;
		ISoundEffect3D** ppMappedSoundEffect = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedSoundEffect = &s_SoundEffects[guid]; //Creates new entry if not existing
		}

		(*ppMappedSoundEffect) = ResourceLoader::LoadSoundEffectFromFile(pFilepath);

		return guid;
	}

	void ResourceManager::ReloadAllShaders()
	{
		for (auto it = s_Shaders.begin(); it != s_Shaders.end(); it++)
		{
			if (it->second != nullptr)
			{
				ShaderLoadDesc loadDesc = s_ShaderLoadConfigurations[it->first];

				IShader* pShader = ResourceLoader::LoadShaderFromFile(loadDesc.pFilepath, loadDesc.Stage, loadDesc.Lang, loadDesc.pEntryPoint);

				if (pShader != nullptr)
				{
					SAFERELEASE(it->second);
					it->second = pShader;
				}
			}
		}
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

	ITexture* ResourceManager::GetTexture(GUID_Lambda guid)
	{
		auto it = s_Textures.find(guid);

		if (it != s_Textures.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTexture called with invalid GUID %u", guid);
		return nullptr;
	}

	ITextureView* ResourceManager::GetTextureView(GUID_Lambda guid)
	{
		auto it = s_TextureViews.find(guid);

		if (it != s_TextureViews.end())
			return it->second;

		D_LOG_WARNING("[ResourceManager]: GetTextureView called with invalid GUID %u", guid);
		return nullptr;
	}

	IShader* ResourceManager::GetShader(GUID_Lambda guid)
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

	GUID_Lambda ResourceManager::RegisterLoadedMesh(Mesh* pResource)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh** ppMappedResource = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			ppMappedResource = &s_Meshes[guid]; //Creates new entry if not existing
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
			ppMappedResource = &s_Materials[guid]; //Creates new entry if not existing
		}

		pResource->pAlbedoMap					= pResource->pAlbedoMap					!= nullptr ? pResource->pAlbedoMap					: s_Textures[DEFAULT_COLOR_MAP];
		pResource->pNormalMap					= pResource->pNormalMap					!= nullptr ? pResource->pNormalMap					: s_Textures[DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMap			= pResource->pAmbientOcclusionMap		!= nullptr ? pResource->pAmbientOcclusionMap		: s_Textures[DEFAULT_COLOR_MAP];
		pResource->pMetallicMap					= pResource->pMetallicMap				!= nullptr ? pResource->pMetallicMap				: s_Textures[DEFAULT_COLOR_MAP];
		pResource->pRoughnessMap				= pResource->pRoughnessMap				!= nullptr ? pResource->pRoughnessMap				: s_Textures[DEFAULT_COLOR_MAP];
		
		pResource->pAlbedoMapView				= pResource->pAlbedoMapView				!= nullptr ? pResource->pAlbedoMapView				: s_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pNormalMapView				= pResource->pNormalMapView				!= nullptr ? pResource->pNormalMapView				: s_TextureViews[DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMapView		= pResource->pAmbientOcclusionMapView	!= nullptr ? pResource->pAmbientOcclusionMapView	: s_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pMetallicMapView				= pResource->pMetallicMapView			!= nullptr ? pResource->pMetallicMapView			: s_TextureViews[DEFAULT_COLOR_MAP];
		pResource->pRoughnessMapView			= pResource->pRoughnessMapView			!= nullptr ? pResource->pRoughnessMapView			: s_TextureViews[DEFAULT_COLOR_MAP];

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
			ppMappedTexture = &s_Textures[guid]; //Creates new entry if not existing
			ppMappedTextureView = &s_TextureViews[guid]; //Creates new entry if not existing
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

		(*ppMappedTextureView) = RenderSystem::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	void ResourceManager::InitDefaultResources()
	{
		s_Meshes[GUID_NONE]					= nullptr;
		s_Materials[GUID_NONE]				= nullptr;
		s_Textures[GUID_NONE]				= nullptr;
        s_Shaders[GUID_NONE]                = nullptr;

		byte defaultColor[4]				= { 255, 255, 255, 255 };
		byte defaultNormal[4]				= { 127, 127, 127, 0   };
		ITexture* pDefaultColorMap			= ResourceLoader::LoadTextureFromMemory("Default Color Map", defaultColor, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, false);
		ITexture* pDefaultNormalMap			= ResourceLoader::LoadTextureFromMemory("Default Normal Map", defaultNormal, 1, 1, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, false);

		s_Textures[DEFAULT_COLOR_MAP]		= pDefaultColorMap;
		s_Textures[DEFAULT_NORMAL_MAP]		= pDefaultNormalMap;

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

		s_TextureViews[DEFAULT_COLOR_MAP]		= RenderSystem::GetDevice()->CreateTextureView(&defaultColorMapViewDesc);
		s_TextureViews[DEFAULT_NORMAL_MAP]		= RenderSystem::GetDevice()->CreateTextureView(&defaultNormalMapViewDesc);

		Material* pDefaultMaterial = DBG_NEW Material();
		pDefaultMaterial->pAlbedoMap				= s_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pNormalMap				= s_Textures[DEFAULT_NORMAL_MAP];
		pDefaultMaterial->pAmbientOcclusionMap		= s_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pMetallicMap				= s_Textures[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pRoughnessMap				= s_Textures[DEFAULT_COLOR_MAP];

		pDefaultMaterial->pAlbedoMapView			= s_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pNormalMapView			= s_TextureViews[DEFAULT_NORMAL_MAP];
		pDefaultMaterial->pAmbientOcclusionMapView	= s_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pMetallicMapView			= s_TextureViews[DEFAULT_COLOR_MAP];
		pDefaultMaterial->pRoughnessMapView			= s_TextureViews[DEFAULT_COLOR_MAP];

		s_Materials[DEFAULT_MATERIAL] = pDefaultMaterial;

	}
}
