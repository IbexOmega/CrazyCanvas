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

		EventQueue::RegisterEventHandler<ShaderRecompileEvent>(&OnShaderRecompileEvent);

		return true;
	}

	bool ResourceManager::Release()
	{
		EventQueue::UnregisterEventHandler<ShaderRecompileEvent>(&OnShaderRecompileEvent);

		SAFEDELETE_ALL(s_Meshes);
		SAFEDELETE_ALL(s_Materials);
		SAFERELEASE_ALL(s_Textures);
		SAFERELEASE_ALL(s_TextureViews);
		SAFERELEASE_ALL(s_Shaders);
		SAFEDELETE_ALL(s_SoundEffects);

		return true;
	}

	bool ResourceManager::LoadSceneFromFile(const String& filename, TArray<MeshComponent>& result)
	{
		TArray<MeshComponent>	sceneLocalMeshComponents;
		TArray<Mesh*>			meshes;
		TArray<Animation*>		animations;
		TArray<Material*>		materials;
		TArray<Texture*>		textures;

		if (!ResourceLoader::LoadSceneFromFile(SCENE_DIR + filename, sceneLocalMeshComponents, meshes, animations, materials, textures))
		{
			return false;
		}

		result = sceneLocalMeshComponents;
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
			GUID_Lambda guid = RegisterLoadedMesh("Scene Mesh " + std::to_string(i), meshes[i]);
			for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
			{
				if (sceneLocalMeshComponents[g].MeshGUID == i)
				{
					result[g].MeshGUID = guid;
				}
			}
		}

		for (uint32 i = 0; i < materials.GetSize(); i++)
		{
			GUID_Lambda guid = RegisterLoadedMaterial("Scene Material " + std::to_string(i), materials[i]);
			CombineMaterials(materials[i], guid);
			for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
			{
				if (sceneLocalMeshComponents[g].MaterialGUID == i)
				{
					result[g].MaterialGUID = guid;
				}
			}
		}

		for (uint32 g = 0; g < sceneLocalMeshComponents.GetSize(); g++)
		{
			if (sceneLocalMeshComponents[g].MeshGUID >= meshes.GetSize())
			{
				LOG_ERROR("[ResourceManager]: GameObject %u in Scene %s has no Mesh", g, filename.c_str());
			}

			if (sceneLocalMeshComponents[g].MaterialGUID >= materials.GetSize())
			{
				result[g].MaterialGUID = GUID_MATERIAL_DEFAULT;
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

		Texture* pAlbedoMap						= albedoMap				!= GUID_NONE ? s_Textures[albedoMap]				: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pNormalMap						= normalMap				!= GUID_NONE ? s_Textures[normalMap]				: s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		Texture* pAmbientOcclusionMap			= ambientOcclusionMap	!= GUID_NONE ? s_Textures[ambientOcclusionMap]		: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pMetallicMap					= metallicMap			!= GUID_NONE ? s_Textures[metallicMap]				: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		Texture* pRoughnessMap					= roughnessMap			!= GUID_NONE ? s_Textures[roughnessMap]				: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		TextureView* pAlbedoMapView				= albedoMap				!= GUID_NONE ? s_TextureViews[albedoMap]			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pNormalMapView				= normalMap				!= GUID_NONE ? s_TextureViews[normalMap]			: s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		TextureView* pAmbientOcclusionMapView	= ambientOcclusionMap	!= GUID_NONE ? s_TextureViews[ambientOcclusionMap]	: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pMetallicMapView			= metallicMap			!= GUID_NONE ? s_TextureViews[metallicMap]			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		TextureView* pRoughnessMapView			= roughnessMap			!= GUID_NONE ? s_TextureViews[roughnessMap]			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

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

		CombineMaterials(*ppMappedMaterial, guid);

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

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

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

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

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

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		return guid;
	}

	GUID_Lambda ResourceManager::LoadShaderFromFile(const String& filename, FShaderStageFlag stage, EShaderLang lang, const char* pEntryPoint)
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

	void ResourceManager::CombineMaterials(Material* pMaterial, GUID_Lambda guid)
	{
		// Find largest texture size
		uint32 largestHeight = std::max(pMaterial->pMetallicMap->GetDesc().Width, std::max(pMaterial->pRoughnessMap->GetDesc().Width, pMaterial->pAmbientOcclusionMap->GetDesc().Width));
		uint32 largestWidth = std::max(pMaterial->pMetallicMap->GetDesc().Height, std::max(pMaterial->pRoughnessMap->GetDesc().Height, pMaterial->pAmbientOcclusionMap->GetDesc().Height));

		// Create Command List
		CommandAllocator* computeCmdAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Combine Material Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		CommandAllocator* graphicsCmdAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Combine Material Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

		CommandListDesc commandListDesc = { };
		commandListDesc.DebugName			= "Compute Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		CommandList* computeCmdList = RenderAPI::GetDevice()->CreateCommandList(computeCmdAllocator, &commandListDesc);

		commandListDesc.DebugName = "Graphics Command List";
		CommandList* graphicsCmdList = RenderAPI::GetDevice()->CreateCommandList(graphicsCmdAllocator, &commandListDesc);
		computeCmdList->Begin(nullptr);

		FenceDesc fenceDesc = {};
		fenceDesc.DebugName		= "CombineMaterials Fence";
		fenceDesc.InitalValue	= 0;

		Fence* pFence = RenderAPI::GetDevice()->CreateFence(&fenceDesc);

		// Create Textures
		uint32_t miplevels = 1u;
		miplevels = uint32(glm::floor(glm::log2((float)glm::max(largestWidth, largestHeight)))) + 1u;

		TextureDesc textureDesc = { };
		textureDesc.DebugName		= "CombineMaterial Texture";
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

		s_Textures[guid] = RenderAPI::GetDevice()->CreateTexture(&textureDesc);

		TextureViewDesc textureViewDesc;
		textureViewDesc.DebugName		= "CombineMaterial TextureView";
		textureViewDesc.pTexture		= s_Textures[guid];
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS;
		textureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= miplevels;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		s_TextureViews[guid] = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		if (s_Textures[guid] == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create texture for \"Combine Material Texture Desc\"");
			return;
		}

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = { };
		transitionToCopyDstBarrier.pTexture					= s_Textures[guid];
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_GENERAL;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= textureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= textureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= textureDesc.ArrayCount;

		computeCmdList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);

		// Create Sampler
		SamplerDesc samplerDesc	= { };
		samplerDesc.DebugName				= "CombineMaterial Sampler ";
		samplerDesc.MinFilter				= EFilterType::FILTER_TYPE_LINEAR;
		samplerDesc.MagFilter				= EFilterType::FILTER_TYPE_LINEAR;
		samplerDesc.MipmapMode				= EMipmapMode::MIPMAP_MODE_LINEAR;
		samplerDesc.AddressModeU			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.AddressModeV			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.AddressModeW			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.MipLODBias				= 0.0f;
		samplerDesc.AnisotropyEnabled		= false;
		samplerDesc.MaxAnisotropy			= 16.0f;
		samplerDesc.MinLOD					= 0.0f;
		samplerDesc.MaxLOD					= FLT32_MAX;

		TSharedRef<Sampler> sampler = RenderAPI::GetDevice()->CreateSampler(&samplerDesc);

		// Create Pipelinelayout
		DescriptorBindingDesc ubo_roughness_mat = { };
		ubo_roughness_mat.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		ubo_roughness_mat.DescriptorCount		= 1;
		ubo_roughness_mat.Binding				= 0;
		ubo_roughness_mat.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		ubo_roughness_mat.ImmutableSamplers		= { sampler };

		DescriptorBindingDesc ubo_metallic_mat = { };
		ubo_metallic_mat.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		ubo_metallic_mat.DescriptorCount		= 1;
		ubo_metallic_mat.Binding				= 1;
		ubo_metallic_mat.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		ubo_metallic_mat.ImmutableSamplers		= { sampler };

		DescriptorBindingDesc ubo_ao_material = { };
		ubo_ao_material.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		ubo_ao_material.DescriptorCount			= 1;
		ubo_ao_material.Binding					= 2;
		ubo_ao_material.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		ubo_ao_material.ImmutableSamplers		= { sampler };

		DescriptorBindingDesc ubo_output_image = { };
		ubo_output_image.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE;
		ubo_output_image.DescriptorCount		= 1;
		ubo_output_image.Binding				= 3;
		ubo_output_image.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc = { };
		descriptorSetLayoutDesc.DescriptorBindings = { ubo_roughness_mat, ubo_metallic_mat, ubo_ao_material, ubo_output_image };

		PipelineLayoutDesc pPipelineLayoutDesc = { };
		pPipelineLayoutDesc.DebugName				= "CombineMaterial PipelineLayout";
		pPipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc };

		PipelineLayout* pPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pPipelineLayoutDesc);

		// Create DescriptorSet
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount	= descriptorSetLayoutDesc.DescriptorBindings.GetSize();

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName				= "CombineMaterial Descriptor";
		descriptorHeapDesc.DescriptorSetCount		= 1;
		descriptorHeapDesc.DescriptorCount			= descriptorCountDesc;

		DescriptorHeap* pDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);

		DescriptorSet* pDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Combine Material Descriptor Set", pPipelineLayout, 0, pDescriptorHeap);
		pDescriptorSet->WriteTextureDescriptors(&pMaterial->pRoughnessMapView, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
		pDescriptorSet->WriteTextureDescriptors(&pMaterial->pMetallicMapView, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
		pDescriptorSet->WriteTextureDescriptors(&pMaterial->pAmbientOcclusionMapView, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 2, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER);
		pDescriptorSet->WriteTextureDescriptors(&s_TextureViews[guid], &sampler, ETextureState::TEXTURE_STATE_GENERAL, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE);

		// Create Shaders
		GUID_Lambda computeShaderGUID = LoadShaderFromFile("CombineMaterial.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL, "main");

		ShaderModuleDesc shaderModuleDesc = { };
		shaderModuleDesc.pShader = s_Shaders[computeShaderGUID];

		ComputePipelineStateDesc computePipelineStateDesc = { };
		computePipelineStateDesc.DebugName			= "CombineMaterial PipelineState";
		computePipelineStateDesc.pPipelineLayout	= pPipelineLayout;
		computePipelineStateDesc.Shader				= shaderModuleDesc;

		PipelineState* pPipelineState = RenderAPI::GetDevice()->CreateComputePipelineState(&computePipelineStateDesc);
		computeCmdList->BindDescriptorSetCompute(pDescriptorSet, pPipelineLayout, 0);
		computeCmdList->BindComputePipeline(pPipelineState);

		// Dispatch
		largestWidth	= std::max<uint32>(largestWidth / 8, 1);
		largestHeight	= std::max<uint32>(largestHeight / 8, 1);

		computeCmdList->Dispatch(largestWidth, largestHeight, 1);

		computeCmdList->QueueTransferBarrier(s_Textures[guid], FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM,
			FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE, 0, ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE, ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		computeCmdList->End();
		RenderAPI::GetComputeQueue()->ExecuteCommandLists(&computeCmdList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, pFence, 1);

		graphicsCmdList->Begin(nullptr);

		graphicsCmdList->QueueTransferBarrier(s_Textures[guid], FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY,
			FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE, 0, ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE, ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		graphicsCmdList->GenerateMiplevels(s_Textures[guid], ETextureState::TEXTURE_STATE_GENERAL, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);

		graphicsCmdList->End();

		RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&graphicsCmdList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, pFence, 1, pFence, 2);
		pFence->Wait(2, UINT64_MAX);

		pMaterial->pCombinedMaterialMap			= s_Textures[guid];
		pMaterial->pCombinedMaterialMapView		= s_TextureViews[guid];

		pPipelineState->Release();
		pDescriptorSet->Release();
		pDescriptorHeap->Release();
		pPipelineLayout->Release();
		pFence->Release();
		computeCmdList->Release();
		graphicsCmdList->Release();
		computeCmdAllocator->Release();
		graphicsCmdAllocator->Release();
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

	bool ResourceManager::OnShaderRecompileEvent(const ShaderRecompileEvent& event)
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

		return true;
	}

	GUID_Lambda ResourceManager::RegisterLoadedMesh(const String& name, Mesh* pResource)
	{
		VALIDATE(pResource != nullptr);

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
		VALIDATE(pResource != nullptr);

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
		pResource->pRoughnessMap				= pResource->pCombinedMaterialMap		!= nullptr ? pResource->pCombinedMaterialMap		: s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		pResource->pAlbedoMapView				= pResource->pAlbedoMapView				!= nullptr ? pResource->pAlbedoMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pNormalMapView				= pResource->pNormalMapView				!= nullptr ? pResource->pNormalMapView				: s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
		pResource->pAmbientOcclusionMapView		= pResource->pAmbientOcclusionMapView	!= nullptr ? pResource->pAmbientOcclusionMapView	: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pMetallicMapView				= pResource->pMetallicMapView			!= nullptr ? pResource->pMetallicMapView			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pRoughnessMapView			= pResource->pRoughnessMapView			!= nullptr ? pResource->pRoughnessMapView			: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
		pResource->pCombinedMaterialMapView		= pResource->pCombinedMaterialMapView	!= nullptr ? pResource->pCombinedMaterialMapView	: s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

		(*ppMappedResource) = pResource;

		return guid;
	}

	GUID_Lambda ResourceManager::RegisterLoadedTexture(Texture* pResource)
	{
		VALIDATE(pResource != nullptr);

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

		(*ppMappedTextureView) = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

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

			s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP]		= RenderAPI::GetDevice()->CreateTextureView(&defaultColorMapViewDesc);
			s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP]		= RenderAPI::GetDevice()->CreateTextureView(&defaultNormalMapViewDesc);
		}

		{
			Material* pDefaultMaterial = DBG_NEW Material();
			pDefaultMaterial->pAlbedoMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMap				= s_Textures[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAmbientOcclusionMap		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pMetallicMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pRoughnessMap				= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pCombinedMaterialMap		= s_Textures[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			pDefaultMaterial->pAlbedoMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pNormalMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_NORMAL_MAP];
			pDefaultMaterial->pAmbientOcclusionMapView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pMetallicMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pRoughnessMapView			= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];
			pDefaultMaterial->pCombinedMaterialMapView	= s_TextureViews[GUID_TEXTURE_DEFAULT_COLOR_MAP];

			s_MaterialNamesToGUIDs["Default Material"]	= GUID_MATERIAL_DEFAULT;
			s_Materials[GUID_MATERIAL_DEFAULT] = pDefaultMaterial;
		}
	}
}
