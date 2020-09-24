#include "Resources/ResourceLoader.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Rendering/RenderAPI.h"

#include "Audio/AudioSystem.h"

#include "Resources/STB.h"

#include "Log/Log.h"

#include "Containers/THashTable.h"
#include "Containers/TUniquePtr.h"

#include <glslangStandAlone/DirStackFileIncluder.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/MachineIndependent/reflection.h>

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
	*  --------------------------glslang Helpers Begin---------------------------------
	*/

	static const TBuiltInResource* GetDefaultBuiltInResources()
	{
		static TBuiltInResource defaultBuiltInResources = {};

		defaultBuiltInResources.maxLights									= 32;
		defaultBuiltInResources.maxClipPlanes								= 6;
		defaultBuiltInResources.maxTextureUnits								= 32;
		defaultBuiltInResources.maxTextureCoords							= 32;
		defaultBuiltInResources.maxVertexAttribs							= 64;
		defaultBuiltInResources.maxVertexUniformComponents					= 4096;
		defaultBuiltInResources.maxVaryingFloats							= 64;
		defaultBuiltInResources.maxVertexTextureImageUnits					= 32;
		defaultBuiltInResources.maxCombinedTextureImageUnits				= 80;
		defaultBuiltInResources.maxTextureImageUnits						= 32;
		defaultBuiltInResources.maxFragmentUniformComponents				= 4096;
		defaultBuiltInResources.maxDrawBuffers								= 32;
		defaultBuiltInResources.maxVertexUniformVectors						= 128;
		defaultBuiltInResources.maxVaryingVectors							= 8;
		defaultBuiltInResources.maxFragmentUniformVectors					= 16;
		defaultBuiltInResources.maxVertexOutputVectors						= 16;
		defaultBuiltInResources.maxFragmentInputVectors						= 15;
		defaultBuiltInResources.minProgramTexelOffset						= -8;
		defaultBuiltInResources.maxProgramTexelOffset						= 7;
		defaultBuiltInResources.maxClipDistances							= 8;
		defaultBuiltInResources.maxComputeWorkGroupCountX					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupCountY					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupCountZ					= 65535;
		defaultBuiltInResources.maxComputeWorkGroupSizeX					= 1024;
		defaultBuiltInResources.maxComputeWorkGroupSizeY					= 1024;
		defaultBuiltInResources.maxComputeWorkGroupSizeZ					= 64;
		defaultBuiltInResources.maxComputeUniformComponents					= 1024;
		defaultBuiltInResources.maxComputeTextureImageUnits					= 16;
		defaultBuiltInResources.maxComputeImageUniforms						= 8;
		defaultBuiltInResources.maxComputeAtomicCounters					= 8;
		defaultBuiltInResources.maxComputeAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxVaryingComponents						= 60;
		defaultBuiltInResources.maxVertexOutputComponents					= 64;
		defaultBuiltInResources.maxGeometryInputComponents					= 64;
		defaultBuiltInResources.maxGeometryOutputComponents					= 128;
		defaultBuiltInResources.maxFragmentInputComponents					= 128;
		defaultBuiltInResources.maxImageUnits								= 8;
		defaultBuiltInResources.maxCombinedImageUnitsAndFragmentOutputs		= 8;
		defaultBuiltInResources.maxCombinedShaderOutputResources			= 8;
		defaultBuiltInResources.maxImageSamples								= 0;
		defaultBuiltInResources.maxVertexImageUniforms						= 0;
		defaultBuiltInResources.maxTessControlImageUniforms					= 0;
		defaultBuiltInResources.maxTessEvaluationImageUniforms				= 0;
		defaultBuiltInResources.maxGeometryImageUniforms					= 0;
		defaultBuiltInResources.maxFragmentImageUniforms					= 8;
		defaultBuiltInResources.maxCombinedImageUniforms					= 8;
		defaultBuiltInResources.maxGeometryTextureImageUnits				= 16;
		defaultBuiltInResources.maxGeometryOutputVertices					= 256;
		defaultBuiltInResources.maxGeometryTotalOutputComponents			= 1024;
		defaultBuiltInResources.maxGeometryUniformComponents				= 1024;
		defaultBuiltInResources.maxGeometryVaryingComponents				= 64;
		defaultBuiltInResources.maxTessControlInputComponents				= 128;
		defaultBuiltInResources.maxTessControlOutputComponents				= 128;
		defaultBuiltInResources.maxTessControlTextureImageUnits				= 16;
		defaultBuiltInResources.maxTessControlUniformComponents				= 1024;
		defaultBuiltInResources.maxTessControlTotalOutputComponents			= 4096;
		defaultBuiltInResources.maxTessEvaluationInputComponents			= 128;
		defaultBuiltInResources.maxTessEvaluationOutputComponents			= 128;
		defaultBuiltInResources.maxTessEvaluationTextureImageUnits			= 16;
		defaultBuiltInResources.maxTessEvaluationUniformComponents			= 1024;
		defaultBuiltInResources.maxTessPatchComponents						= 120;
		defaultBuiltInResources.maxPatchVertices							= 32;
		defaultBuiltInResources.maxTessGenLevel								= 64;
		defaultBuiltInResources.maxViewports								= 16;
		defaultBuiltInResources.maxVertexAtomicCounters						= 0;
		defaultBuiltInResources.maxTessControlAtomicCounters				= 0;
		defaultBuiltInResources.maxTessEvaluationAtomicCounters				= 0;
		defaultBuiltInResources.maxGeometryAtomicCounters					= 0;
		defaultBuiltInResources.maxFragmentAtomicCounters					= 8;
		defaultBuiltInResources.maxCombinedAtomicCounters					= 8;
		defaultBuiltInResources.maxAtomicCounterBindings					= 1;
		defaultBuiltInResources.maxVertexAtomicCounterBuffers				= 0;
		defaultBuiltInResources.maxTessControlAtomicCounterBuffers			= 0;
		defaultBuiltInResources.maxTessEvaluationAtomicCounterBuffers		= 0;
		defaultBuiltInResources.maxGeometryAtomicCounterBuffers				= 0;
		defaultBuiltInResources.maxFragmentAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxCombinedAtomicCounterBuffers				= 1;
		defaultBuiltInResources.maxAtomicCounterBufferSize					= 16384;
		defaultBuiltInResources.maxTransformFeedbackBuffers					= 4;
		defaultBuiltInResources.maxTransformFeedbackInterleavedComponents	= 64;
		defaultBuiltInResources.maxCullDistances							= 8;
		defaultBuiltInResources.maxCombinedClipAndCullDistances				= 8;
		defaultBuiltInResources.maxSamples									= 4;
		defaultBuiltInResources.limits.nonInductiveForLoops					= true;
		defaultBuiltInResources.limits.whileLoops							= true;
		defaultBuiltInResources.limits.doWhileLoops							= true;
		defaultBuiltInResources.limits.generalUniformIndexing				= true;
		defaultBuiltInResources.limits.generalAttributeMatrixVectorIndexing = true;
		defaultBuiltInResources.limits.generalVaryingIndexing				= true;
		defaultBuiltInResources.limits.generalSamplerIndexing				= true;
		defaultBuiltInResources.limits.generalVariableIndexing				= true;
		defaultBuiltInResources.limits.generalConstantMatrixVectorIndexing	= true;

		// Mesh shaders
		defaultBuiltInResources.maxMeshWorkGroupSizeX_NV	= 32;
		defaultBuiltInResources.maxMeshWorkGroupSizeY_NV	= 1;
		defaultBuiltInResources.maxMeshWorkGroupSizeZ_NV	= 1;
		defaultBuiltInResources.maxTaskWorkGroupSizeX_NV	= 32;
		defaultBuiltInResources.maxTaskWorkGroupSizeY_NV	= 1;
		defaultBuiltInResources.maxTaskWorkGroupSizeZ_NV	= 1;
		defaultBuiltInResources.maxMeshOutputVerticesNV		= 256;
		defaultBuiltInResources.maxMeshOutputPrimitivesNV	= 512;
		defaultBuiltInResources.maxMeshViewCountNV			= 4;

		return &defaultBuiltInResources;
	}

	static EShLanguage ConvertShaderStageToEShLanguage(FShaderStageFlags shaderStage)
	{
		switch (shaderStage)
		{
		case FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER:			return EShLanguage::EShLangMeshNV;
		case FShaderStageFlag::SHADER_STAGE_FLAG_TASK_SHADER:			return EShLanguage::EShLangTaskNV;
		case FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER:			return EShLanguage::EShLangVertex;
		case FShaderStageFlag::SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return EShLanguage::EShLangGeometry;
		case FShaderStageFlag::SHADER_STAGE_FLAG_HULL_SHADER:			return EShLanguage::EShLangTessControl;
		case FShaderStageFlag::SHADER_STAGE_FLAG_DOMAIN_SHADER:			return EShLanguage::EShLangTessEvaluation;
		case FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER:			return EShLanguage::EShLangFragment;
		case FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER:		return EShLanguage::EShLangCompute;
		case FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER:			return EShLanguage::EShLangRayGen;
		case FShaderStageFlag::SHADER_STAGE_FLAG_INTERSECT_SHADER:		return EShLanguage::EShLangIntersect;
		case FShaderStageFlag::SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return EShLanguage::EShLangAnyHit;
		case FShaderStageFlag::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return EShLanguage::EShLangClosestHit;
		case FShaderStageFlag::SHADER_STAGE_FLAG_MISS_SHADER:			return EShLanguage::EShLangMiss;

		case FShaderStageFlag::SHADER_STAGE_FLAG_NONE:
		default:
			return EShLanguage::EShLangCount;
		}
	}

	/*
	*  --------------------------glslang Helpers End---------------------------------
	*/

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
	static Texture* LoadAssimpTexture(SceneLoadingContext& context, const aiMaterial* pMaterial, aiTextureType type, uint32 index)
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
				Texture* pTexture = ResourceLoader::LoadTextureArrayFromFile(name, context.DirectoryPath, &name, 1, EFormat::FORMAT_R8G8B8A8_UNORM, true);
				context.LoadedTextures[name] = pTexture;
				return context.pTextures->PushBack(pTexture);
			}
			else
			{
				return loadedTexture->second;
			}
		}

		return nullptr;
	}

	bool ResourceLoader::LoadSceneFromFile(const String& filepath, TArray<MeshComponent>& meshComponents, TArray<Mesh*>& meshes, TArray<Material*>& materials, TArray<Texture*>& textures)
	{
		int32 assimpFlags =
			aiProcess_FlipUVs					|
			aiProcess_CalcTangentSpace			|
			aiProcess_FindInstances				|
			aiProcess_GenSmoothNormals			|
			aiProcess_JoinIdenticalVertices		|
			aiProcess_ImproveCacheLocality		|
			aiProcess_LimitBoneWeights			|
			aiProcess_RemoveRedundantMaterials	|
			aiProcess_SplitLargeMeshes			|
			aiProcess_Triangulate				|
			aiProcess_GenUVCoords				|
			aiProcess_SortByPType				|
			aiProcess_FindDegenerates			|
			aiProcess_FindInvalidData;

		SceneLoadRequest loadRequest = {
			.Filepath		= ConvertSlashes(filepath),
			.AssimpFlags	= assimpFlags,
			.Meshes			= meshes,
			.MeshComponents	= meshComponents,
			.pMaterials		= &materials,
			.pTextures		= &textures
		};

		return LoadSceneWithAssimp(loadRequest);
	}

	Mesh* ResourceLoader::LoadMeshFromFile(const String& filepath)
	{
		int32 assimpFlags =
			aiProcess_FlipUVs					|
			aiProcess_CalcTangentSpace			|
			aiProcess_FindInstances				|
			aiProcess_GenSmoothNormals			|
			aiProcess_JoinIdenticalVertices		|
			aiProcess_ImproveCacheLocality		|
			aiProcess_LimitBoneWeights			|
			aiProcess_RemoveRedundantMaterials	|
			aiProcess_SplitLargeMeshes			|
			aiProcess_Triangulate				|
			aiProcess_GenUVCoords				|
			aiProcess_SortByPType				|
			aiProcess_FindDegenerates			|
			aiProcess_OptimizeMeshes			|
			aiProcess_OptimizeGraph				|
			aiProcess_FindInvalidData;

		TArray<Mesh*> meshes;
		TArray<MeshComponent> meshComponent;

		SceneLoadRequest loadRequest = {
			.Filepath		= ConvertSlashes(filepath),
			.AssimpFlags	= assimpFlags,
			.Meshes			= meshes,
			.MeshComponents	= meshComponent,
			.pMaterials		= nullptr,
			.pTextures		= nullptr,
		};

		if (!LoadSceneWithAssimp(loadRequest))
			return nullptr;

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Mesh \"%s\"", filepath.c_str());
		return meshes.GetFront();
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
		int texWidth = 0;
		int texHeight = 0;
		int bpp = 0;

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
			D_LOG_MESSAGE("[ResourceLoader]: Loaded Texture \"%s\"", filepath.c_str());
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

		BufferDesc bufferDesc	= {};
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

	Shader* ResourceLoader::LoadShaderFromFile(const String& filepath, FShaderStageFlags stage, EShaderLang lang, const String& entryPoint)
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

	Shader* ResourceLoader::LoadShaderFromMemory(const String& source, const String& name, FShaderStageFlags stage, EShaderLang lang, const String& entryPoint)
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

	bool ResourceLoader::CreateShaderReflection(const String& filepath, FShaderStageFlags stage, EShaderLang lang, ShaderReflection* pReflection)
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

		ISoundEffect3D* pSound = AudioSystem::GetDevice()->CreateSoundEffect(&soundDesc);
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
		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TShader shader(shaderType);

		std::string source			= std::string(pSource);
		int32 foundBracket			= int32(source.find_last_of('}') + 1);
		source[foundBracket]		= '\0';
		const char* pFinalSource	= source.c_str();
		shader.setStringsWithLengths(&pFinalSource, &foundBracket, 1);

		//Todo: Fetch this
		int32 clientInputSemanticsVersion					= 100;
		glslang::EShTargetClientVersion vulkanClientVersion	= glslang::EShTargetVulkan_1_2;
		glslang::EShTargetLanguageVersion targetVersion		= glslang::EShTargetSpv_1_5;

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

		const TBuiltInResource* pResources	= GetDefaultBuiltInResources();
		EShMessages messages				= static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		const int defaultVersion			= 450;

		DirStackFileIncluder includer;

		//Get Directory Path of File
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
		for (uint32 vertexIdx = 0; vertexIdx < pMeshAI->mNumVertices; vertexIdx++)
		{
			Vertex vertex;
			vertex.Position.x = pMeshAI->mVertices[vertexIdx].x;
			vertex.Position.y = pMeshAI->mVertices[vertexIdx].y;
			vertex.Position.z = pMeshAI->mVertices[vertexIdx].z;

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

		TArray<Mesh::IndexType> indices;
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
		memcpy(pMesh->Indices.GetData(), indices.GetData(), sizeof(Mesh::IndexType) * indices.GetSize());
	}

	void ResourceLoader::LoadMaterial(SceneLoadingContext& context, const aiScene* pSceneAI, const aiMesh* pMeshAI)
	{
		auto mat = context.MaterialIndices.find(pMeshAI->mMaterialIndex);
		if (mat == context.MaterialIndices.end())
		{
			Material*	pMaterial	= DBG_NEW Material();
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

		VALIDATE(pScene != nullptr);

		SceneLoadingContext context = {
			.DirectoryPath	= filepath.substr(0, lastPathDivisor + 1),
			.Meshes			= sceneLoadRequest.Meshes,
			.MeshComponents	= sceneLoadRequest.MeshComponents,
			.pMaterials		= sceneLoadRequest.pMaterials,
			.pTextures		= sceneLoadRequest.pTextures
		};


		ProcessAssimpNode(context, pScene->mRootNode, pScene);
		return true;
	}

	void ResourceLoader::ProcessAssimpNode(SceneLoadingContext& context, const aiNode* pNode, const aiScene* pScene)
	{
		context.Meshes.Reserve(context.Meshes.GetSize() + pNode->mNumMeshes);

		for (uint32 meshIdx = 0; meshIdx < pNode->mNumMeshes; meshIdx++)
		{
			Mesh* pMesh = DBG_NEW Mesh;
			aiMesh* pMeshAI = pScene->mMeshes[pNode->mMeshes[meshIdx]];

			LoadVertices(pMesh, pMeshAI);
			LoadIndices(pMesh, pMeshAI);

			if (context.pMaterials)
			{
				LoadMaterial(context, pScene, pMeshAI);
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
