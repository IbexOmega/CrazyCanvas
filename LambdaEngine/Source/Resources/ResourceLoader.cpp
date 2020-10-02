#include "Resources/ResourceLoader.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Rendering/RenderAPI.h"

#include "Audio/AudioAPI.h"

#include "Resources/STB.h"
#include "Resources/GLSLShaderSource.h"

#include "Log/Log.h"

#include "Containers/THashTable.h"
#include "Containers/TUniquePtr.h"

#include "Resources/GLSLang.h"

#include <cstdio>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace LambdaEngine
{
	CommandAllocator*		ResourceLoader::s_pCopyCommandAllocator		= nullptr;
	CommandList*			ResourceLoader::s_pCopyCommandList			= nullptr;
	Fence*					ResourceLoader::s_pCopyFence				= nullptr;
	uint64					ResourceLoader::s_SignalValue				= 1;

	/*
	* Helpers
	*/
	static void ConvertSlashes(String& string)
	{
		{
			size_t pos = string.find_first_of('\\');
			while (pos != String::npos)
			{
				string.replace(pos, 1, 1, '/');
				pos = string.find_first_of('\\', pos + 1);
			}
		}

		{
			size_t pos = string.find_first_of('/');
			while (pos != String::npos)
			{
				size_t afterPos = pos + 1;
				if (string[afterPos] == '/')
				{
					string.erase(string.begin() + afterPos);
				}

				pos = string.find_first_of('/', afterPos);
			}
		}
	}

	static String ConvertSlashes(const String& string)
	{
		String result = string;
		{
			size_t pos = result.find_first_of('\\');
			while (pos != std::string::npos)
			{
				result.replace(pos, 1, 1, '/');
				pos = result.find_first_of('\\', pos + 1);
			}
		}

		{
			size_t pos = result.find_first_of('/');
			while (pos != std::string::npos)
			{
				size_t afterPos = pos + 1;
				if (result[afterPos] == '/')
				{
					result.erase(result.begin() + afterPos);
				}

				pos = result.find_first_of('/', afterPos);
			}
		}

		return result;
	}

	// Removes extra data after the fileending (Some materials has extra data after file ending)
	static void RemoveExtraData(String& string)
	{
		size_t dotPos = string.find_first_of('.');
		size_t endPos = string.find_first_of(' ', dotPos);
		if (dotPos != String::npos && endPos != String::npos)
		{
			string = string.substr(0, endPos);
		}
	}

	static FLoadedTextureFlag AssimpTextureFlagToLambdaTextureFlag(aiTextureType textureType)
	{
		switch (textureType)
		{
		case aiTextureType::aiTextureType_DIFFUSE:		return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ALBEDO;
		case aiTextureType::aiTextureType_NORMALS:		return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL;
		case aiTextureType::aiTextureType_HEIGHT:		return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NORMAL;
		case aiTextureType::aiTextureType_AMBIENT:		return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_AO;
		case aiTextureType::aiTextureType_REFLECTION:	return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_METALLIC;
		case aiTextureType::aiTextureType_SHININESS:	return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_ROUGHNESS;
		}

		return FLoadedTextureFlag::LOADED_TEXTURE_FLAG_NONE;
	}

	/*
	* ResourceLoader
	*/
	bool ResourceLoader::Init()
	{
		s_pCopyCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Resource Loader Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		if (s_pCopyCommandAllocator == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Could not create Copy Command Allocator");
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName		= "Resource Loader Copy Command List";
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		s_pCopyCommandList = RenderAPI::GetDevice()->CreateCommandList(s_pCopyCommandAllocator, &commandListDesc);

		FenceDesc fenceDesc = {};
		fenceDesc.DebugName		= "Resource Loader Copy Fence";
		fenceDesc.InitalValue	= 0;
		s_pCopyFence = RenderAPI::GetDevice()->CreateFence(&fenceDesc);

		glslang::InitializeProcess();

		return true;
	}

	bool ResourceLoader::Release()
	{
		SAFERELEASE(s_pCopyCommandAllocator);
		SAFERELEASE(s_pCopyCommandList);
		SAFERELEASE(s_pCopyFence);

		glslang::FinalizeProcess();

		return true;
	}

	/*
	* Assimp Parsing
	*/
	static LoadedTexture* LoadAssimpTexture(SceneLoadingContext& context, const aiMaterial* pMaterial, aiTextureType type, uint32 index)
	{
		if (pMaterial->GetTextureCount(type) > index)
		{
			aiString str;
			pMaterial->GetTexture(type, index, &str);

			String name = str.C_Str();
			ConvertSlashes(name);
			RemoveExtraData(name);

			auto loadedTexture = context.LoadedTextures.find(name);
			if (loadedTexture == context.LoadedTextures.end())
			{
				LoadedTexture* pLoadedTexture = DBG_NEW LoadedTexture();
				pLoadedTexture->pTexture	= ResourceLoader::LoadTextureArrayFromFile(name, context.DirectoryPath, &name, 1, EFormat::FORMAT_R8G8B8A8_UNORM, true);
				pLoadedTexture->Flags = AssimpTextureFlagToLambdaTextureFlag(type);

				context.LoadedTextures[name] = pLoadedTexture;
				return context.pTextures->PushBack(pLoadedTexture);
			}
			else
			{
				return loadedTexture->second;
			}
		}

		return nullptr;
	}

	bool ResourceLoader::LoadSceneFromFile(
		const String& filepath,
		TArray<MeshComponent>& meshComponents,
		TArray<Mesh*>& meshes,
		TArray<Animation*>& animations,
		TArray<LoadedMaterial*>& materials,
		TArray<LoadedTexture*>& textures)
	{
		int32 assimpFlags =
			aiProcess_FlipWindingOrder			|
			aiProcess_FlipUVs					|
			aiProcess_CalcTangentSpace			|
			aiProcess_FindInstances				|
			aiProcess_GenSmoothNormals			|
			aiProcess_JoinIdenticalVertices		|
			aiProcess_ImproveCacheLocality		|
			aiProcess_LimitBoneWeights			|
			aiProcess_SplitLargeMeshes			|
			aiProcess_RemoveRedundantMaterials	|
			aiProcess_SortByPType				|
			aiProcess_Triangulate				|
			aiProcess_GenUVCoords				|
			aiProcess_FindDegenerates			|
			aiProcess_OptimizeMeshes			|
			aiProcess_OptimizeGraph				|
			aiProcess_FindInvalidData;

		// Prevent crashes in assimp when using this flag
		String path = ConvertSlashes(filepath);
		if (path.find(".obj") == String::npos)
		{
			//assimpFlags |= aiProcess_PopulateArmatureData;
		}

		SceneLoadRequest loadRequest = 
		{
			.Filepath		= ConvertSlashes(filepath),
			.AssimpFlags	= assimpFlags,
			.Meshes			= meshes,
			.Animations		= animations,
			.MeshComponents	= meshComponents,
			.pMaterials		= &materials,
			.pTextures		= &textures
		};

		return LoadSceneWithAssimp(loadRequest);
	}

	Mesh* ResourceLoader::LoadMeshFromFile(const String& filepath, TArray<Animation*>& animations)
	{
		int32 assimpFlags = 
			aiProcess_FlipWindingOrder			|
			aiProcess_FlipUVs					|
			aiProcess_CalcTangentSpace			|
			aiProcess_FindInstances				|
			aiProcess_GenSmoothNormals			|
			aiProcess_JoinIdenticalVertices		|
			aiProcess_ImproveCacheLocality		|
			aiProcess_LimitBoneWeights			|
			aiProcess_RemoveRedundantMaterials	|
			aiProcess_Triangulate				|
			aiProcess_GenUVCoords				|
			aiProcess_FindDegenerates			|
			aiProcess_OptimizeMeshes			|
			aiProcess_OptimizeGraph				|
			//aiProcess_SortByPType				|
			aiProcess_FindInvalidData;
		
		// Prevent crashes in assimp when using this flag
		String path = ConvertSlashes(filepath);
		if (path.find(".obj") == String::npos)
		{
			assimpFlags |= aiProcess_PopulateArmatureData;
		}

		TArray<Mesh*>			meshes;
		TArray<MeshComponent>	meshComponent;

		SceneLoadRequest loadRequest =
		{
			.Filepath		= ConvertSlashes(filepath),
			.AssimpFlags	= assimpFlags,
			.Meshes			= meshes,
			.Animations		= animations,
			.MeshComponents = meshComponent,
			.pMaterials		= nullptr,
			.pTextures		= nullptr,
		};

		if (!LoadSceneWithAssimp(loadRequest))
		{
			return nullptr;
		}

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Mesh \"%s\"", filepath.c_str());

		// Find the largest and delete the ones not used
		uint32 biggest	= 0;
		uint32 maxCount	= 0;
		for (uint32 i = 0; i < meshes.GetSize(); i++)
		{
			if (meshes[i]->Vertices.GetSize() > maxCount)
			{
				biggest		= i;
				maxCount	= meshes[i]->Vertices.GetSize();
			}
		}

		for (Mesh* pMesh : meshes)
		{
			if (meshes[biggest] != pMesh)
			{
				SAFEDELETE(pMesh);
			}
		}

		return meshes[biggest];
	}

	Mesh* ResourceLoader::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		Mesh* pMesh = DBG_NEW Mesh();
		pMesh->Vertices.Resize(numVertices);
		memcpy(pMesh->Vertices.GetData(), pVertices, sizeof(Vertex) * numVertices);

		pMesh->Indices.Resize(numVertices);
		memcpy(pMesh->Indices.GetData(), pIndices, sizeof(uint32) * numIndices);

		MeshFactory::GenerateMeshlets(pMesh, MAX_VERTS, MAX_PRIMS);
		return pMesh;
	}

	Texture* ResourceLoader::LoadTextureArrayFromFile(const String& name, const String& dir, const String* pFilenames, uint32 count, EFormat format, bool generateMips)
	{
		int texWidth	= 0;
		int texHeight	= 0;
		int bpp			= 0;

		TArray<void*> stbi_pixels(count);
		for (uint32 i = 0; i < count; i++)
		{
			String filepath = dir + ConvertSlashes(pFilenames[i]);

			void* pPixels = nullptr;

			if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
			{
				pPixels = (void*)stbi_load(filepath.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
			}
			else if (format == EFormat::FORMAT_R16_UNORM)
			{
				pPixels = (void*)stbi_load_16(filepath.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
			}
			else
			{
				LOG_ERROR("[ResourceLoader]: Texture format not supported for \"%s\"", filepath.c_str());
				return nullptr;
			}

			if (pPixels == nullptr)
			{
				LOG_ERROR("[ResourceLoader]: Failed to load texture file: \"%s\"", filepath.c_str());
				return nullptr;
			}

			stbi_pixels[i] = pPixels;
			// D_LOG_MESSAGE("[ResourceLoader]: Loaded Texture \"%s\"", filepath.c_str());
		}

		Texture* pTexture = nullptr;

		if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
		{
			pTexture = LoadTextureArrayFromMemory(name, stbi_pixels.GetData(), stbi_pixels.GetSize(), texWidth, texHeight, format, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, generateMips);
		}
		else if (format == EFormat::FORMAT_R16_UNORM)
		{
			TArray<void*> pixels(count * 4);

			for (uint32 i = 0; i < count; i++)
			{
				uint32 numPixels = texWidth * texHeight;
				uint16* pPixelsR = DBG_NEW uint16[numPixels];
				uint16* pPixelsG = DBG_NEW uint16[numPixels];
				uint16* pPixelsB = DBG_NEW uint16[numPixels];
				uint16* pPixelsA = DBG_NEW uint16[numPixels];

				uint16* pSTBIPixels = reinterpret_cast<uint16*>(stbi_pixels[i]);

				for (uint32 p = 0; p < numPixels; p++)
				{
					pPixelsR[p] = pSTBIPixels[4 * p + 0];
					pPixelsG[p] = pSTBIPixels[4 * p + 1];
					pPixelsB[p] = pSTBIPixels[4 * p + 2];
					pPixelsA[p] = pSTBIPixels[4 * p + 3];
				}

				pixels[4 * i + 0] = pPixelsR;
				pixels[4 * i + 1] = pPixelsG;
				pixels[4 * i + 2] = pPixelsB;
				pixels[4 * i + 3] = pPixelsA;
			}

			pTexture = LoadTextureArrayFromMemory(name, pixels.GetData(), pixels.GetSize(), texWidth, texHeight, format, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE, generateMips);

			for (uint32 i = 0; i < pixels.GetSize(); i++)
			{
				uint16* pPixels = reinterpret_cast<uint16*>(pixels[i]);
				SAFEDELETE_ARRAY(pPixels);
			}
		}

		for (uint32 i = 0; i < count; i++)
		{
			stbi_image_free(stbi_pixels[i]);
		}

		return pTexture;
	}

	Texture* ResourceLoader::LoadCubeTexturesArrayFromFile(const String& name, const String& dir, const String* pFilenames, uint32 count, EFormat format, bool generateMips)
	{
		int texWidth = 0;
		int texHeight = 0;
		int bpp = 0;

		const uint32 textureCount = count;
		TArray<void*> stbi_pixels(textureCount);

		for (uint32 i = 0; i < textureCount; i++)
		{
			String filepath = dir + ConvertSlashes(pFilenames[i]);

			void* pPixels = nullptr;

			if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
			{
				pPixels = (void*)stbi_load(filepath.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
			}
			else
			{
				LOG_ERROR("[ResourceLoader]: Texture format not supported for \"%s\"", filepath.c_str());
				return nullptr;
			}

			if (pPixels == nullptr)
			{
				LOG_ERROR("[ResourceLoader]: Failed to load texture file: \"%s\"", filepath.c_str());
				return nullptr;
			}

			stbi_pixels[i] = pPixels;
			D_LOG_MESSAGE("[ResourceLoader]: Loaded Texture \"%s\"", filepath.c_str());
		}

		Texture* pTexture = nullptr;

		if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
		{
			uint32 flags = FTextureFlag::TEXTURE_FLAG_CUBE_COMPATIBLE | FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE;
			pTexture = LoadTextureArrayFromMemory(name, stbi_pixels.GetData(), stbi_pixels.GetSize(), texWidth, texHeight, format, flags, generateMips);
		}

		for (uint32 i = 0; i < textureCount; i++)
		{
			stbi_image_free(stbi_pixels[i]);
		}

		return pTexture;
	}

	Texture* ResourceLoader::LoadTextureArrayFromMemory(const String& name, const void* const * ppData, uint32 arrayCount, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips)
	{
		uint32_t miplevels = 1u;

		if (generateMips)
		{
			miplevels = uint32(glm::floor(glm::log2((float)glm::max(width, height)))) + 1u;
		}

		TextureDesc textureDesc = {};
		textureDesc.DebugName	= name;
		textureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		textureDesc.Format		= format;
		textureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		textureDesc.Flags		= FTextureFlag::TEXTURE_FLAG_COPY_SRC | FTextureFlag::TEXTURE_FLAG_COPY_DST | (FTextureFlag)usageFlags;
		textureDesc.Width		= width;
		textureDesc.Height		= height;
		textureDesc.Depth		= 1;
		textureDesc.ArrayCount	= arrayCount;
		textureDesc.Miplevels	= miplevels;
		textureDesc.SampleCount = 1;

		Texture* pTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);

		if (pTexture == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create texture for \"%s\"", name.c_str());
			return nullptr;
		}

		uint32 pixelDataSize = width * height * TextureFormatStride(format);

		BufferDesc bufferDesc	= { };
		bufferDesc.DebugName	= "Texture Copy Buffer";
		bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		bufferDesc.SizeInBytes	= uint64(arrayCount * pixelDataSize);

		Buffer* pTextureData = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		if (pTextureData == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create copy buffer for \"%s\"", name.c_str());
			return nullptr;
		}

		const uint64 waitValue = s_SignalValue - 1;
		s_pCopyFence->Wait(waitValue, UINT64_MAX);

		s_pCopyCommandAllocator->Reset();
		s_pCopyCommandList->Begin(nullptr);

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture					= pTexture;
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= textureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= textureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= textureDesc.ArrayCount;

		s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);

		for (uint32 i = 0; i < arrayCount; i++)
		{
			uint64 bufferOffset = uint64(i) * pixelDataSize;

			void* pTextureDataDst = pTextureData->Map();
			const void* pTextureDataSrc = ppData[i];
			memcpy((void*)(uint64(pTextureDataDst) + bufferOffset), pTextureDataSrc, pixelDataSize);
			pTextureData->Unmap();

			CopyTextureFromBufferDesc copyDesc = {};
			copyDesc.SrcOffset		= bufferOffset;
			copyDesc.SrcRowPitch	= 0;
			copyDesc.SrcHeight		= 0;
			copyDesc.Width			= width;
			copyDesc.Height			= height;
			copyDesc.Depth			= 1;
			copyDesc.Miplevel		= 0;
			copyDesc.MiplevelCount  = miplevels;
			copyDesc.ArrayIndex		= i;
			copyDesc.ArrayCount		= 1;

			s_pCopyCommandList->CopyTextureFromBuffer(pTextureData, pTexture, copyDesc);
		}

		if (generateMips)
		{
			s_pCopyCommandList->GenerateMiplevels(pTexture, ETextureState::TEXTURE_STATE_COPY_DST, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
		}
		else
		{
			PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
			transitionToShaderReadBarrier.pTexture				= pTexture;
			transitionToShaderReadBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
			transitionToShaderReadBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			transitionToShaderReadBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			transitionToShaderReadBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			transitionToShaderReadBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			transitionToShaderReadBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			transitionToShaderReadBarrier.TextureFlags			= textureDesc.Flags;
			transitionToShaderReadBarrier.Miplevel				= 0;
			transitionToShaderReadBarrier.MiplevelCount			= textureDesc.Miplevels;
			transitionToShaderReadBarrier.ArrayIndex			= 0;
			transitionToShaderReadBarrier.ArrayCount			= textureDesc.ArrayCount;

			s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);
		}

		s_pCopyCommandList->End();

		if (!RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&s_pCopyCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, s_pCopyFence, s_SignalValue))
		{
			LOG_ERROR("[ResourceLoader]: Texture could not be created as command list could not be executed for \"%s\"", name.c_str());
			SAFERELEASE(pTextureData);

			return nullptr;
		}
		else
		{
			s_SignalValue++;
		}

		//Todo: Remove this wait after garbage collection works
		RenderAPI::GetGraphicsQueue()->Flush();

		SAFERELEASE(pTextureData);

		return pTexture;
	}

	Shader* ResourceLoader::LoadShaderFromFile(const String& filepath, FShaderStageFlag stage, EShaderLang lang, const String& entryPoint)
	{
		String file = ConvertSlashes(filepath);

		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		TArray<uint32> sourceSPIRV;
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(file, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", file.c_str());
				return nullptr;
			}

			if (!CompileGLSLToSPIRV(file, reinterpret_cast<char*>(pShaderRawSource), stage, &sourceSPIRV, nullptr))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", file.c_str());
				return nullptr;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			if (!ReadDataFromFile(file, "rb", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", file.c_str());
				return nullptr;
			}

			sourceSPIRV.Resize(static_cast<uint32>(glm::ceil(static_cast<float32>(shaderRawSourceSize) / sizeof(uint32))));
			memcpy(sourceSPIRV.GetData(), pShaderRawSource, shaderRawSourceSize);
		}

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= file;
		shaderDesc.Source		= TArray<byte>(reinterpret_cast<byte*>(sourceSPIRV.GetData()), reinterpret_cast<byte*>(sourceSPIRV.GetData()) + sourceSize);
		shaderDesc.EntryPoint	= entryPoint;
		shaderDesc.Stage		= stage;
		shaderDesc.Lang			= lang;

		Shader* pShader = RenderAPI::GetDevice()->CreateShader(&shaderDesc);
		Malloc::Free(pShaderRawSource);

		return pShader;
	}

	Shader* ResourceLoader::LoadShaderFromMemory(const String& source, const String& name, FShaderStageFlag stage, EShaderLang lang, const String& entryPoint)
	{
		TArray<uint32> sourceSPIRV;
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!CompileGLSLToSPIRV("", source.c_str(), stage, &sourceSPIRV, nullptr))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV");
				return nullptr;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			sourceSPIRV.Resize(static_cast<uint32>(glm::ceil(static_cast<float32>(source.size()) / sizeof(uint32))));
			memcpy(sourceSPIRV.GetData(), source.data(), source.size());
		}

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= name;
		shaderDesc.Source		= TArray<byte>(reinterpret_cast<byte*>(sourceSPIRV.GetData()), reinterpret_cast<byte*>(sourceSPIRV.GetData()) + sourceSize);
		shaderDesc.EntryPoint	= entryPoint;
		shaderDesc.Stage		= stage;
		shaderDesc.Lang			= lang;

		Shader* pShader = RenderAPI::GetDevice()->CreateShader(&shaderDesc);

		return pShader;
	}

	GLSLShaderSource ResourceLoader::LoadShaderSourceFromFile(const String& filepath, FShaderStageFlag stage, const String& entryPoint)
	{
		if (stage == FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER ||
			stage == FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER ||
			stage == FShaderStageFlag::SHADER_STAGE_FLAG_ANY_HIT_SHADER ||
			stage == FShaderStageFlag::SHADER_STAGE_FLAG_INTERSECT_SHADER ||
			stage == FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER)
		{
			VALIDATE_MSG(false, "[ResourceLoader]: Unsupported shader stage because GLSLang can't get their shit together");
		}

		String file = ConvertSlashes(filepath);

		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		if (!ReadDataFromFile(file, "r", &pShaderRawSource, &shaderRawSourceSize))
		{
			LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", file.c_str());
			return nullptr;
		}

		size_t found = file.find_last_of("/");
		std::string shaderName = file.substr(found + 1, file.length());
		std::string directoryPath = file.substr(0, found);

		GLSLShaderSourceDesc shaderSourceDesc = {};
		shaderSourceDesc.Name			= shaderName;
		shaderSourceDesc.EntryPoint		= entryPoint;
		shaderSourceDesc.ShaderStage	= stage;
		shaderSourceDesc.Source			= reinterpret_cast<const char*>(pShaderRawSource);
		shaderSourceDesc.Directory		= directoryPath;

		GLSLShaderSource shaderSource(&shaderSourceDesc);
		Malloc::Free(pShaderRawSource);

		return shaderSource;
	}

	bool ResourceLoader::CreateShaderReflection(const String& filepath, FShaderStageFlag stage, EShaderLang lang, ShaderReflection* pReflection)
	{
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		String path = ConvertSlashes(filepath);
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(path, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", path.c_str());
				return false;
			}

			if (!CompileGLSLToSPIRV(path, reinterpret_cast<char*>(pShaderRawSource), stage, nullptr, pReflection))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", path.c_str());
				return false;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			LOG_ERROR("[ResourceLoader]: CreateShaderReflection currently not supported for SPIRV source language");
			return false;
		}

		Malloc::Free(pShaderRawSource);

		return true;
	}

	ISoundEffect3D* ResourceLoader::LoadSoundEffectFromFile(const String& filepath)
	{
		SoundEffect3DDesc soundDesc = {};
		soundDesc.Filepath = ConvertSlashes(filepath);

		ISoundEffect3D* pSound = AudioAPI::GetDevice()->CreateSoundEffect(&soundDesc);
		if (pSound == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to initialize sound \"%s\"", filepath.c_str());
			return nullptr;
		}

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Sound \"%s\"", filepath.c_str());

		return pSound;
	}

	bool ResourceLoader::ReadDataFromFile(const String& filepath, const char* pMode, byte** ppData, uint32* pDataSize)
	{
		String path = ConvertSlashes(filepath);
		FILE* pFile = fopen(path.c_str(), pMode);
		if (pFile == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load file \"%s\"", path.c_str());
			return false;
		}

		fseek(pFile, 0, SEEK_END);
		int32 length = ftell(pFile) + 1;
		fseek(pFile, 0, SEEK_SET);

		byte* pData = reinterpret_cast<byte*>(Malloc::Allocate(length * sizeof(byte)));
		ZERO_MEMORY(pData, length * sizeof(byte));

		int32 read = int32(fread(pData, 1, length, pFile));
		if (read == 0)
		{
			LOG_ERROR("[ResourceLoader]: Failed to read file \"%s\"", path.c_str());
			fclose(pFile);
			return false;
		}
		else
		{
			pData[read] = '\0';
		}

		(*ppData)		= pData;
		(*pDataSize)	= length;

		fclose(pFile);
		return true;
	}

	bool ResourceLoader::CompileGLSLToSPIRV(const String& filepath, const char* pSource, FShaderStageFlags stage, TArray<uint32>* pSourceSPIRV, ShaderReflection* pReflection)
	{
		std::string source			= std::string(pSource);
		int32 size					= int32(source.size());
		const char* pFinalSource	= source.c_str();

		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TShader shader(shaderType);

		shader.setStringsWithLengths(&pFinalSource, &size, 1);

		//Todo: Fetch this
		int32 clientInputSemanticsVersion					= GetDefaultClientInputSemanticsVersion();
		glslang::EShTargetClientVersion vulkanClientVersion	= GetDefaultVulkanClientVersion();
		glslang::EShTargetLanguageVersion targetVersion		= GetDefaultSPIRVTargetVersion();
		const TBuiltInResource* pResources					= GetDefaultBuiltInResources();
		EShMessages messages								= GetDefaultMessages();
		int32 defaultVersion								= GetDefaultVersion();

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

		DirStackFileIncluder includer;

		// Get Directory Path of File
		size_t found				= filepath.find_last_of("/\\");
		std::string directoryPath	= filepath.substr(0, found);

		includer.pushExternalLocalDirectory(directoryPath);

		//std::string preprocessedGLSL;
		//if (!shader.preprocess(pResources, defaultVersion, ENoProfile, false, false, messages, &preprocessedGLSL, includer))
		//{
		//	LOG_ERROR("[ResourceLoader]: GLSL Preprocessing failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
		//	return false;
		//}

		//const char* pPreprocessedGLSL = preprocessedGLSL.c_str();
		//shader.setStrings(&pPreprocessedGLSL, 1);

		if (!shader.parse(pResources, defaultVersion, false, messages, includer))
		{
			const char* pShaderInfoLog = shader.getInfoLog();
			const char* pShaderDebugInfo = shader.getInfoDebugLog();
			LOG_ERROR("[ResourceLoader]: GLSL Parsing failed for: \"%s\"\n%s\n%s", filepath.c_str(), pShaderInfoLog, pShaderDebugInfo);
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Linking failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		glslang::TIntermediate* pIntermediate = program.getIntermediate(shaderType);

		String sourcesss = pIntermediate->getSourceText();

		if (pSourceSPIRV != nullptr)
		{
			spv::SpvBuildLogger logger;
			glslang::SpvOptions spvOptions;
			std::vector<uint32> std_sourceSPIRV;
			glslang::GlslangToSpv(*pIntermediate, std_sourceSPIRV, &logger, &spvOptions);
			pSourceSPIRV->Assign(std_sourceSPIRV.data(), std_sourceSPIRV.data() + std_sourceSPIRV.size());
		}

		if (pReflection != nullptr)
		{
			if (!CreateShaderReflection(pIntermediate, stage, pReflection))
			{
				LOG_ERROR("[ResourceLoader]: Failed to Create Shader Reflection");
				return false;
			}
		}

		return true;
	}

	bool ResourceLoader::CreateShaderReflection(glslang::TIntermediate* pIntermediate, FShaderStageFlags stage, ShaderReflection* pReflection)
	{
		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TReflection glslangReflection(EShReflectionOptions::EShReflectionAllIOVariables, shaderType, shaderType);
		glslangReflection.addStage(shaderType, *pIntermediate);

		pReflection->NumAtomicCounters		= glslangReflection.getNumAtomicCounters();
		pReflection->NumBufferVariables		= glslangReflection.getNumBufferVariables();
		pReflection->NumPipeInputs			= glslangReflection.getNumPipeInputs();
		pReflection->NumPipeOutputs			= glslangReflection.getNumPipeOutputs();
		pReflection->NumStorageBuffers		= glslangReflection.getNumStorageBuffers();
		pReflection->NumUniformBlocks		= glslangReflection.getNumUniformBlocks();
		pReflection->NumUniforms			= glslangReflection.getNumUniforms();

		return true;
	}

	void ResourceLoader::LoadVertices(Mesh* pMesh, const aiMesh* pMeshAI)
	{
		pMesh->Vertices.Resize(pMeshAI->mNumVertices);

		glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent;
		halfExtent = glm::vec3(0.0f);

		for (uint32 vertexIdx = 0; vertexIdx < pMeshAI->mNumVertices; vertexIdx++)
		{
			Vertex vertex;
			vertex.Position.x = pMeshAI->mVertices[vertexIdx].x;
			vertex.Position.y = pMeshAI->mVertices[vertexIdx].y;
			vertex.Position.z = pMeshAI->mVertices[vertexIdx].z;

			halfExtent.x = std::max(halfExtent.x, std::abs(vertex.Position.x));
			halfExtent.y = std::max(halfExtent.y, std::abs(vertex.Position.y));
			halfExtent.z = std::max(halfExtent.z, std::abs(vertex.Position.z));

			if (pMeshAI->HasNormals())
			{
				vertex.Normal.x = pMeshAI->mNormals[vertexIdx].x;
				vertex.Normal.y = pMeshAI->mNormals[vertexIdx].y;
				vertex.Normal.z = pMeshAI->mNormals[vertexIdx].z;
			}

			if (pMeshAI->HasTangentsAndBitangents())
			{
				vertex.Tangent.x = pMeshAI->mTangents[vertexIdx].x;
				vertex.Tangent.y = pMeshAI->mTangents[vertexIdx].y;
				vertex.Tangent.z = pMeshAI->mTangents[vertexIdx].z;
			}

			if (pMeshAI->HasTextureCoords(0))
			{
				vertex.TexCoord.x = pMeshAI->mTextureCoords[0][vertexIdx].x;
				vertex.TexCoord.y = pMeshAI->mTextureCoords[0][vertexIdx].y;
			}

			pMesh->Vertices[vertexIdx] = vertex;
		}
	}

	void ResourceLoader::LoadIndices(Mesh* pMesh, const aiMesh* pMeshAI)
	{
		VALIDATE(pMeshAI->HasFaces());

		TArray<MeshIndexType> indices;
		indices.Reserve(pMeshAI->mNumFaces * 3);
		for (uint32 faceIdx = 0; faceIdx < pMeshAI->mNumFaces; faceIdx++)
		{
			aiFace face = pMeshAI->mFaces[faceIdx];
			for (uint32 indexIdx = 0; indexIdx < face.mNumIndices; indexIdx++)
			{
				indices.EmplaceBack(face.mIndices[indexIdx]);
			}
		}

		pMesh->Indices.Resize(indices.GetSize());
		memcpy(pMesh->Indices.GetData(), indices.GetData(), sizeof(MeshIndexType) * indices.GetSize());
	}

	inline glm::mat4 AssimpToGLMMat4(const aiMatrix4x4& mat)
	{
		glm::mat4 retMat;

		retMat[0][0] = mat.a1;
		retMat[0][1] = mat.b1;
		retMat[0][2] = mat.c1;
		retMat[0][3] = mat.d1;

		retMat[1][0] = mat.a2;
		retMat[1][1] = mat.b2;
		retMat[1][2] = mat.c2;
		retMat[1][3] = mat.d2;

		retMat[2][0] = mat.a3;
		retMat[2][1] = mat.b3;
		retMat[2][2] = mat.c3;
		retMat[2][3] = mat.d3;

		retMat[3][0] = mat.a4;
		retMat[3][1] = mat.b4;
		retMat[3][2] = mat.c4;
		retMat[3][3] = mat.d4;

		return retMat;
	}

	static aiNode* FindSkeletalRoot(aiNode* pNode)
	{
		VALIDATE(pNode != nullptr);

		aiNode* pParent = pNode->mParent;
		if (pParent)
		{
			return FindSkeletalRoot(pParent);
		}
		else
		{
			return pNode;
		}
	}

	static void FindSkeletalParent(aiNode* pNode, Skeleton* pSkeleton)
	{
		VALIDATE(pNode		!= nullptr);
		VALIDATE(pSkeleton	!= nullptr);

		String name = pNode->mName.C_Str();

		int32 myID = -1;
		auto it = pSkeleton->BoneMap.find(name);
		if (it != pSkeleton->BoneMap.end())
		{
			myID = it->second;
		}

		LOG_INFO("Name=%s, ID=%d", name.c_str(), myID);

		for (uint32 child = 0; child < pNode->mNumChildren; child++)
		{
			aiNode* pChild = pNode->mChildren[child];
			if (pChild)
			{
				String childName = pChild->mName.C_Str();
				
				auto childIt = pSkeleton->BoneMap.find(childName);
				if (childIt != pSkeleton->BoneMap.end())
				{
					pSkeleton->Bones[childIt->second].ParentBoneIndex = myID;
				}

				FindSkeletalParent(pNode->mChildren[child], pSkeleton);
			}
		}
	}

	void ResourceLoader::LoadSkeleton(Mesh* pMesh, const aiMesh* pMeshAI)
	{
		Skeleton* pSkeleton = DBG_NEW Skeleton();
		pMesh->pSkeleton = pSkeleton;

		// Retrive all the bones
		pSkeleton->Bones.Resize(pMeshAI->mNumBones);
		for (uint32 boneIndex = 0; boneIndex < pMeshAI->mNumBones; boneIndex++)
		{
			Bone& bone = pSkeleton->Bones[boneIndex];
			
			aiBone* pBoneAI = pMeshAI->mBones[boneIndex];
			bone.Name = pBoneAI->mName.C_Str();
			
			auto it = pSkeleton->BoneMap.find(bone.Name);
			if (it != pSkeleton->BoneMap.end())
			{
				LOG_ERROR("[ResourceLoader] Multiple bones with the same name");
				return;
			}
			else
			{
				pSkeleton->BoneMap[bone.Name] = boneIndex;
			}
			
			bone.OffsetTransform = AssimpToGLMMat4(pBoneAI->mOffsetMatrix);
			bone.Weights.Resize(pBoneAI->mNumWeights);
			for (uint32 weightIndex = 0; weightIndex < pBoneAI->mNumWeights; weightIndex++)
			{
				bone.Weights[weightIndex].VertexIndex	= pBoneAI->mWeights[weightIndex].mVertexId;
				bone.Weights[weightIndex].VertexWeight	= pBoneAI->mWeights[weightIndex].mWeight;
			}
		}

		// Retrive the parent index for all nodes with the node
		{
			aiNode* pNodeAI = pMeshAI->mBones[0]->mNode;
			if (pNodeAI)
			{
				aiNode* pRoot = FindSkeletalRoot(pNodeAI);
				FindSkeletalParent(pRoot, pSkeleton);
			}
		}

#if 0
		for (uint32 boneID = 0; boneID < pSkeleton->Bones.GetSize(); boneID++)
		{
			Bone& bone = pSkeleton->Bones[boneID];
			LOG_INFO("Name=%s, MyID=%d, ParentID=%d", bone.Name.GetString().c_str(), boneID, bone.ParentBoneIndex);
		}
#endif

		// Go through and correct mistakes with the armature
		for (uint32 boneIndex = 0; boneIndex < pMeshAI->mNumBones; boneIndex++)
		{
			// We already found the parent
			Bone& bone = pSkeleton->Bones[boneIndex];
			if (bone.ParentBoneIndex != -1)
			{
				continue;
			}

			// Search the armature aswell
			aiNode* pNodeAI = pMeshAI->mBones[boneIndex]->mArmature;
			if (pNodeAI)
			{
				aiNode* pParent = pNodeAI->mParent;
				if (pParent)
				{
					auto it = pSkeleton->BoneMap.find(String(pParent->mName.C_Str()));
					if (it != pSkeleton->BoneMap.end())
					{
						bone.ParentBoneIndex = it->second;
					}
				}
			}
		}

#if 0
		LOG_INFO("-----------------------------------");

		for (uint32 boneID = 0; boneID < pSkeleton->Bones.GetSize(); boneID++)
		{
			Bone& bone = pSkeleton->Bones[boneID];
			LOG_INFO("Name=%s, MyID=%d, ParentID=%d", bone.Name.GetString().c_str(), boneID, bone.ParentBoneIndex);
		}
#endif

		// Set weights
		pMesh->VertexBoneData.Resize(pMesh->Vertices.GetSize());
		for (uint32 boneID = 0; boneID < pSkeleton->Bones.GetSize(); boneID++)
		{
			Bone& bone = pSkeleton->Bones[boneID];
			for (uint32 weightID = 0; weightID < bone.Weights.GetSize(); weightID++)
			{
				const uint32	vertexID	= bone.Weights[weightID].VertexIndex;
				const float32	weight		= bone.Weights[weightID].VertexWeight;

				VertexBoneData& vertex = pMesh->VertexBoneData[vertexID];
				if (vertex.Bone0.BoneID == -1)
				{
					vertex.Bone0.BoneID = boneID;
					vertex.Bone0.Weight = weight;
				}
				else if (vertex.Bone1.BoneID == -1)
				{
					vertex.Bone1.BoneID = boneID;
					vertex.Bone1.Weight = weight;
				}
				else if (vertex.Bone2.BoneID == -1)
				{
					vertex.Bone2.BoneID = boneID;
					vertex.Bone2.Weight = weight;
				}
				else if (vertex.Bone3.BoneID == -1)
				{
					vertex.Bone3.BoneID = boneID;
					vertex.Bone3.Weight = weight;
				}
				else
				{
					LOG_WARNING("More than 4 bones affect this vertex");
				}
			}
		}

#if 0
		for (VertexBoneData& bone : pMesh->VertexBoneData)
		{
			LOG_WARNING("BoneData: [0] ID=%d, weight=%.4f [1] ID=%d, weight=%.4f [2] ID=%d, weight=%.4f [3] ID=%d, weight=%.4f", 
				bone.Bone0.BoneID, bone.Bone0.Weight,
				bone.Bone1.BoneID, bone.Bone1.Weight,
				bone.Bone2.BoneID, bone.Bone2.Weight,
				bone.Bone3.BoneID, bone.Bone3.Weight);
		}
#endif
	}

	void ResourceLoader::LoadMaterial(SceneLoadingContext& context, const aiScene* pSceneAI, const aiMesh* pMeshAI)
	{
		auto mat = context.MaterialIndices.find(pMeshAI->mMaterialIndex);
		if (mat == context.MaterialIndices.end())
		{
			LoadedMaterial*	pMaterial	= DBG_NEW LoadedMaterial();
			aiMaterial* pMaterialAI	= pSceneAI->mMaterials[pMeshAI->mMaterialIndex];
#if 0
			for (uint32 t = 0; t < aiTextureType_UNKNOWN; t++)
			{
				uint32 count = pAiMaterial->GetTextureCount(aiTextureType(t));
				if (count > 0)
				{
					LOG_WARNING("Material %d has %d textures of type: %d", pMesh->mMaterialIndex, count, t);
					for (uint32 m = 0; m < count; m++)
					{
						aiString str;
						pAiMaterial->GetTexture(aiTextureType(t), m, &str);

						LOG_WARNING("#%d path=%s", m, str.C_Str());
					}
				}
			}
#endif
			// Albedo
			aiColor4D diffuse;
			if (aiGetMaterialColor(pMaterialAI, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS)
			{
				pMaterial->Properties.Albedo.r = diffuse.r;
				pMaterial->Properties.Albedo.g = diffuse.g;
				pMaterial->Properties.Albedo.b = diffuse.b;
				pMaterial->Properties.Albedo.a = diffuse.a;
			}
			else
			{
				pMaterial->Properties.Albedo.r = 1.0f;
				pMaterial->Properties.Albedo.g = 1.0f;
				pMaterial->Properties.Albedo.b = 1.0f;
				pMaterial->Properties.Albedo.a = 1.0f;
			}

			// Albedo
			pMaterial->pAlbedoMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_BASE_COLOR, 0);
			if (!pMaterial->pAlbedoMap)
			{
				pMaterial->pAlbedoMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_DIFFUSE, 0);
			}

			// Normal
			pMaterial->pNormalMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_NORMAL_CAMERA, 0);
			if (!pMaterial->pNormalMap)
			{
				pMaterial->pNormalMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_NORMALS, 0);
			}
			if (!pMaterial->pNormalMap)
			{
				pMaterial->pNormalMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_HEIGHT, 0);
			}

			// AO
			pMaterial->pAmbientOcclusionMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_AMBIENT_OCCLUSION, 0);
			if (!pMaterial->pAmbientOcclusionMap)
			{
				pMaterial->pAmbientOcclusionMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_AMBIENT, 0);
			}

			// Metallic
			pMaterial->pMetallicMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_METALNESS, 0);
			if (!pMaterial->pMetallicMap)
			{
				pMaterial->pMetallicMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_REFLECTION, 0);
			}

			// Roughness
			pMaterial->pRoughnessMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_DIFFUSE_ROUGHNESS, 0);
			if (!pMaterial->pRoughnessMap)
			{
				pMaterial->pRoughnessMap = LoadAssimpTexture(context, pMaterialAI, aiTextureType_SHININESS, 0);
			}

			context.pMaterials->EmplaceBack(pMaterial);
			context.MaterialIndices[pMeshAI->mMaterialIndex] = context.pMaterials->GetSize() - 1;
		}
	}

	void ResourceLoader::LoadAnimation(SceneLoadingContext& context, const aiAnimation* pAnimationAI)
	{
		VALIDATE(pAnimationAI != nullptr);

		Animation* pAnimation = DBG_NEW Animation();
		pAnimation->Name			= pAnimationAI->mName.C_Str();
		pAnimation->DurationInTicks	= pAnimationAI->mDuration;
		pAnimation->TicksPerSecond	= static_cast<float64>(pAnimationAI->mTicksPerSecond);
		pAnimationAI->mChannels[0]->mNodeName;

		pAnimation->Channels.Resize(pAnimationAI->mNumChannels);
		for (uint32 channelIndex = 0; channelIndex < pAnimationAI->mNumChannels; channelIndex++)
		{
			aiNodeAnim* pChannel = pAnimationAI->mChannels[channelIndex];
			pAnimation->Channels[channelIndex].Name = pChannel->mNodeName.C_Str();
			
			pAnimation->Channels[channelIndex].Positions.Resize(pChannel->mNumPositionKeys);
			for (uint32 i = 0; i < pChannel->mNumPositionKeys; i++)
			{
				pAnimation->Channels[channelIndex].Positions[i].Time	= pChannel->mPositionKeys[i].mTime;
				pAnimation->Channels[channelIndex].Positions[i].Value.x	= pChannel->mPositionKeys[i].mValue.x;
				pAnimation->Channels[channelIndex].Positions[i].Value.y = pChannel->mPositionKeys[i].mValue.y;
				pAnimation->Channels[channelIndex].Positions[i].Value.z = pChannel->mPositionKeys[i].mValue.z;
			}

			pAnimation->Channels[channelIndex].Scales.Resize(pChannel->mNumScalingKeys);
			for (uint32 i = 0; i < pChannel->mNumScalingKeys; i++)
			{
				pAnimation->Channels[channelIndex].Scales[i].Time		= pChannel->mScalingKeys[i].mTime;
				pAnimation->Channels[channelIndex].Scales[i].Value.x	= pChannel->mScalingKeys[i].mValue.x;
				pAnimation->Channels[channelIndex].Scales[i].Value.y	= pChannel->mScalingKeys[i].mValue.y;
				pAnimation->Channels[channelIndex].Scales[i].Value.z	= pChannel->mScalingKeys[i].mValue.z;
			}

			pAnimation->Channels[channelIndex].Rotations.Resize(pChannel->mNumRotationKeys);
			for (uint32 i = 0; i < pChannel->mNumRotationKeys; i++)
			{
				pAnimation->Channels[channelIndex].Rotations[i].Time	= pChannel->mRotationKeys[i].mTime;
				pAnimation->Channels[channelIndex].Rotations[i].Value.x	= pChannel->mRotationKeys[i].mValue.x;
				pAnimation->Channels[channelIndex].Rotations[i].Value.y	= pChannel->mRotationKeys[i].mValue.y;
				pAnimation->Channels[channelIndex].Rotations[i].Value.z	= pChannel->mRotationKeys[i].mValue.z;
				pAnimation->Channels[channelIndex].Rotations[i].Value.w = pChannel->mRotationKeys[i].mValue.w;
			}
		}

		context.Animations.EmplaceBack(pAnimation);
	}

	bool ResourceLoader::LoadSceneWithAssimp(SceneLoadRequest& sceneLoadRequest)
	{
		// Find the directory path
		const String& filepath = sceneLoadRequest.Filepath;
		size_t lastPathDivisor = filepath.find_last_of("/\\");
		if (lastPathDivisor == String::npos)
		{
			LOG_WARNING("[ResourceLoader]: Failed to load scene '%s'. No parent directory found...", filepath.c_str());
			return false;
		}

		Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(filepath, sceneLoadRequest.AssimpFlags);
		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load scene '%s'. Error: %s", filepath.c_str(), importer.GetErrorString());
			return false;
		}

		SceneLoadingContext context = 
		{
			.DirectoryPath	= filepath.substr(0, lastPathDivisor + 1),
			.Meshes			= sceneLoadRequest.Meshes,
			.MeshComponents	= sceneLoadRequest.MeshComponents,
			.Animations		= sceneLoadRequest.Animations,
			.pMaterials		= sceneLoadRequest.pMaterials,
			.pTextures		= sceneLoadRequest.pTextures
		};

		// Load all meshes
		ProcessAssimpNode(context, pScene->mRootNode, pScene);

		// Load all animations
		if (pScene->mNumAnimations > 0)
		{
			for (uint32 animationIndex = 0; animationIndex < pScene->mNumAnimations; animationIndex++)
			{
				LoadAnimation(context, pScene->mAnimations[animationIndex]);
			}
		}

		return true;
	}

	void ResourceLoader::ProcessAssimpNode(SceneLoadingContext& context, const aiNode* pNode, const aiScene* pScene)
	{
		context.Meshes.Reserve(context.Meshes.GetSize() + pNode->mNumMeshes);
		for (uint32 meshIdx = 0; meshIdx < pNode->mNumMeshes; meshIdx++)
		{
			Mesh*	pMesh	= DBG_NEW Mesh;
			aiMesh*	pMeshAI	= pScene->mMeshes[pNode->mMeshes[meshIdx]];

			LoadVertices(pMesh, pMeshAI);
			LoadIndices(pMesh, pMeshAI);

			if (context.pMaterials)
			{
				LoadMaterial(context, pScene, pMeshAI);
			}

			if (pMeshAI->mNumBones > 0)
			{
				LoadSkeleton(pMesh, pMeshAI);
				if (pMesh->pSkeleton)
				{
					pMesh->pSkeleton->GlobalTransform = AssimpToGLMMat4(pNode->mTransformation);
				}
			}

			MeshFactory::GenerateMeshlets(pMesh, MAX_VERTS, MAX_PRIMS);

			context.Meshes.EmplaceBack(pMesh);

			MeshComponent newMeshComponent;
			newMeshComponent.MeshGUID		= context.Meshes.GetSize() - 1;
			newMeshComponent.MaterialGUID	= context.MaterialIndices[pMeshAI->mMaterialIndex];
			context.MeshComponents.PushBack(newMeshComponent);
		}

		for (uint32 childIdx = 0; childIdx < pNode->mNumChildren; childIdx++)
		{
			ProcessAssimpNode(context, pNode->mChildren[childIdx], pScene);
		}
	}
}
