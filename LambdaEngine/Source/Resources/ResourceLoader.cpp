#include "Resources/ResourceLoader.h"

#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/IFence.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

#include "Rendering/RenderSystem.h"

#include "Audio/AudioSystem.h"

#include "Resources/STB.h"

#include "Log/Log.h"

#include "Containers/THashTable.h"

#include <glslangStandAlone/DirStackFileIncluder.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>

#include <tiny_obj_loader.h>
#include <cstdio>

namespace LambdaEngine
{
	IDeviceAllocator*		ResourceLoader::s_pAllocator				= nullptr;
	ICommandAllocator*		ResourceLoader::s_pCopyCommandAllocator		= nullptr;
	ICommandList*			ResourceLoader::s_pCopyCommandList			= nullptr;
	IFence*					ResourceLoader::s_pCopyFence				= nullptr;
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

		return &defaultBuiltInResources;
	}

	static EShLanguage ConvertShaderStageToEShLanguage(FShaderStageFlags shaderStage)
	{
		switch (shaderStage)
		{
		case FShaderStageFlags::SHADER_STAGE_FLAG_MESH_SHADER:			return EShLanguage::EShLangMeshNV;
		case FShaderStageFlags::SHADER_STAGE_FLAG_TASK_SHADER:			return EShLanguage::EShLangTaskNV;
		case FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER:		return EShLanguage::EShLangVertex;
		case FShaderStageFlags::SHADER_STAGE_FLAG_GEOMETRY_SHADER:		return EShLanguage::EShLangGeometry;
		case FShaderStageFlags::SHADER_STAGE_FLAG_HULL_SHADER:			return EShLanguage::EShLangTessControl;
		case FShaderStageFlags::SHADER_STAGE_FLAG_DOMAIN_SHADER:		return EShLanguage::EShLangTessEvaluation;
		case FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER:			return EShLanguage::EShLangFragment;
		case FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER:		return EShLanguage::EShLangCompute;
		case FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER:		return EShLanguage::EShLangRayGen;
		case FShaderStageFlags::SHADER_STAGE_FLAG_INTERSECT_SHADER:		return EShLanguage::EShLangIntersect;
		case FShaderStageFlags::SHADER_STAGE_FLAG_ANY_HIT_SHADER:		return EShLanguage::EShLangAnyHit;
		case FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER:	return EShLanguage::EShLangClosestHit;
		case FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER:			return EShLanguage::EShLangMiss;

		case FShaderStageFlags::SHADER_STAGE_FLAG_NONE:
		default:
			return EShLanguage::EShLangCount;
		}
	}

	/*
	*  --------------------------glslang Helpers End---------------------------------
	*/

	bool ResourceLoader::Init()
	{
		s_pCopyCommandAllocator = RenderSystem::GetDevice()->CreateCommandAllocator("Resource Loader Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);

		if (s_pCopyCommandAllocator == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Could not create Copy Command Allocator");
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.pName			= "Resource Loader Copy Command List";
		commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;
		commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		s_pCopyCommandList = RenderSystem::GetDevice()->CreateCommandList(s_pCopyCommandAllocator, &commandListDesc);

		FenceDesc fenceDesc = {};
		fenceDesc.pName			= "Resource Loader Copy Fence";
		fenceDesc.InitalValue	= 0;
		s_pCopyFence = RenderSystem::GetDevice()->CreateFence(&fenceDesc);

		DeviceAllocatorDesc allocatorDesc = {};
		allocatorDesc.pName				= "Resource Allocator";
		allocatorDesc.PageSizeInBytes	= MEGA_BYTE(64);
		s_pAllocator = RenderSystem::GetDevice()->CreateDeviceAllocator(&allocatorDesc);

		glslang::InitializeProcess();

		return true;
	}

	bool ResourceLoader::Release()
	{
		SAFERELEASE(s_pAllocator);
		SAFERELEASE(s_pCopyCommandAllocator);
		SAFERELEASE(s_pCopyCommandList);
		SAFERELEASE(s_pCopyFence);

		glslang::FinalizeProcess();

		return true;
	}

	bool ResourceLoader::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GameObject>& loadedGameObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures)
	{
		std::string filepath = std::string(pDir) + std::string(pFilename);

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn;
		std::string err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filepath.c_str(), pDir, true, false))
		{
            LOG_WARNING("[ResourceDevice]: Failed to load scene '%s'. Warning: %s Error: %s", filepath.c_str(), warn.c_str(), err.c_str());
			return false;
		}

		loadedMeshes.resize(shapes.size());
		loadedMaterials.resize(materials.size());

		std::unordered_map<std::string, ITexture*> loadedTexturesMap;

		for (uint32 m = 0; m < materials.size(); m++)
		{
			tinyobj::material_t& material = materials[m];

			Material* pMaterial = DBG_NEW Material();

			if (material.diffuse_texname.length() > 0)
			{
				std::string texturePath = pDir + material.diffuse_texname;
                ConvertBackslashes(texturePath);

                auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(texturePath.c_str(), EFormat::FORMAT_R8G8B8A8_UNORM, true);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pAlbedoMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pAlbedoMap = loadedTexture->second;
				}
			}

			if (material.bump_texname.length() > 0)
			{
				std::string texturePath = pDir + material.bump_texname;
                ConvertBackslashes(texturePath);
                
                auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(texturePath.c_str(), EFormat::FORMAT_R8G8B8A8_UNORM, true);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pNormalMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pNormalMap = loadedTexture->second;
				}
			}

			if (material.ambient_texname.length() > 0)
			{
				std::string texturePath = pDir + material.ambient_texname;
                ConvertBackslashes(texturePath);
                
                auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(texturePath.c_str(), EFormat::FORMAT_R8G8B8A8_UNORM, true);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pMetallicMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pMetallicMap = loadedTexture->second;
				}
			}

			if (material.specular_highlight_texname.length() > 0)
			{
				std::string texturePath = pDir + material.specular_highlight_texname;
                ConvertBackslashes(texturePath);
                
                auto loadedTexture = loadedTexturesMap.find(texturePath);
				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(texturePath.c_str(), EFormat::FORMAT_R8G8B8A8_UNORM, true);
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pRoughnessMap		= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pRoughnessMap = loadedTexture->second;
				}
			}

			loadedMaterials[m] = pMaterial;
		}

		for (uint32 s = 0; s < shapes.size(); s++)
		{
			tinyobj::shape_t& shape = shapes[s];

			std::vector<Vertex> vertices = {};
			std::vector<uint32> indices = {};
			std::unordered_map<Vertex, uint32> uniqueVertices = {};

			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * (size_t)index.vertex_index + 0],
					attributes.vertices[3 * (size_t)index.vertex_index + 1],
					attributes.vertices[3 * (size_t)index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					vertex.Normal =
					{
						attributes.normals[3 * (size_t)index.normal_index + 0],
						attributes.normals[3 * (size_t)index.normal_index + 1],
						attributes.normals[3 * (size_t)index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord =
					{
						attributes.texcoords[2 * (size_t)index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * (size_t)index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}

			//Calculate tangents
			for (uint32 index = 0; index < indices.size(); index += 3)
			{
				Vertex& v0 = vertices[indices[(size_t)index + 0]];
				Vertex& v1 = vertices[indices[(size_t)index + 1]];
				Vertex& v2 = vertices[indices[(size_t)index + 2]];

				v0.CalculateTangent(v1, v2);
				v1.CalculateTangent(v2, v0);
				v2.CalculateTangent(v0, v1);
			}

			Mesh* pMesh = LoadMeshFromMemory(vertices.data(), uint32(vertices.size()), indices.data(), uint32(indices.size()));
			loadedMeshes[s] = pMesh;

			D_LOG_MESSAGE("[ResourceDevice]: Loaded Mesh \"%s\" \t for scene : \"%s\"", shape.name.c_str(), pFilename);

			uint32 m = shape.mesh.material_ids[0];

			GameObject gameObject	= {};
			gameObject.Mesh			= s;
			gameObject.Material		= m;

			loadedGameObjects.push_back(gameObject);
		}

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Scene \"%s\"", pFilename);

		return true;
	}

	Mesh* ResourceLoader::LoadMeshFromFile(const char* pFilepath)
	{
		//Start New Thread

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, pFilepath, nullptr, true, false))
		{
			LOG_WARNING("[ResourceDevice]: Failed to load mesh '%s'. Warning: %s Error: %s", pFilepath, warn.c_str(), err.c_str());
			return nullptr;
		}

		std::vector<Vertex> vertices = {};
		std::vector<uint32> indices = {};
		std::unordered_map<Vertex, uint32> uniqueVertices = {};

		for (const tinyobj::shape_t& shape : shapes)
		{
			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * index.vertex_index + 0],
					attributes.vertices[3 * index.vertex_index + 1],
					attributes.vertices[3 * index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					vertex.Normal =
					{
						attributes.normals[3 * index.normal_index + 0],
						attributes.normals[3 * index.normal_index + 1],
						attributes.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord =
					{
						attributes.texcoords[2 * index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		//Calculate tangents
		for (uint32 index = 0; index < indices.size(); index += 3)
		{
			Vertex& v0 = vertices[indices[index + 0]];
			Vertex& v1 = vertices[indices[index + 1]];
			Vertex& v2 = vertices[indices[index + 2]];

			v0.CalculateTangent(v1, v2);
			v1.CalculateTangent(v2, v0);
			v2.CalculateTangent(v0, v1);
		}

		Mesh* pMesh = LoadMeshFromMemory(vertices.data(), uint32(vertices.size()), indices.data(), uint32(indices.size()));

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Mesh \"%s\"", pFilepath);

		return pMesh;
	}

	Mesh* ResourceLoader::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		Vertex* pVertexArray = DBG_NEW Vertex[numVertices];
		memcpy(pVertexArray, pVertices, sizeof(Vertex) * numVertices);

		uint32* pIndexArray = DBG_NEW uint32[numIndices];
		memcpy(pIndexArray, pIndices, sizeof(uint32) * numIndices);

		Mesh* pMesh = DBG_NEW Mesh();
		pMesh->pVertexArray		= pVertexArray;
		pMesh->pIndexArray		= pIndexArray;
		pMesh->VertexCount		= numVertices;
		pMesh->IndexCount		= numIndices;

		return pMesh;
	}

	ITexture* ResourceLoader::LoadTextureFromFile(const char* pFilepath, EFormat format, bool generateMips)
	{
		int texWidth = 0;
		int texHeight = 0;
		int bpp = 0;

		void* pPixels = nullptr;

		if (format == EFormat::FORMAT_R8G8B8A8_UNORM)
		{
			pPixels = (void*)stbi_load(pFilepath, &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
		}
		/*else if (format == EFormat::FORMAT_R32G32B32A32_FLOAT)
		{
			pPixels = (void*)stbi_loadf(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
		}*/
		else
		{
			LOG_ERROR("[ResourceLoader]: Texture format not supported for \"%s\"", pFilepath);
			return nullptr;
		}

		if (pPixels == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load texture file: \"%s\"", pFilepath);
			return nullptr;
		}

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Texture \"%s\"", pFilepath);

		ITexture* pTexture = LoadTextureFromMemory(pFilepath, pPixels, texWidth, texHeight, format, FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE, generateMips);
		stbi_image_free(pPixels);

		return pTexture;
	}

	ITexture* ResourceLoader::LoadTextureFromMemory(const char* pName, const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips)
	{
		uint32_t miplevels = 1u;

		if (generateMips)
		{
			miplevels = uint32(glm::floor(glm::log2((float)glm::max(width, height)))) + 1u;
		}

		TextureDesc textureDesc = {};
		textureDesc.pName		= pName;
		textureDesc.MemoryType	= EMemoryType::MEMORY_GPU;
		textureDesc.Format		= format;
		textureDesc.Type		= ETextureType::TEXTURE_2D;
		textureDesc.Flags		= FTextureFlags::TEXTURE_FLAG_COPY_SRC | FTextureFlags::TEXTURE_FLAG_COPY_DST | usageFlags;
		textureDesc.Width		= width;
		textureDesc.Height		= height;
		textureDesc.Depth		= 1;
		textureDesc.ArrayCount	= 1;
		textureDesc.Miplevels	= miplevels;
		textureDesc.SampleCount = 1;

		ITexture* pTexture = RenderSystem::GetDevice()->CreateTexture(&textureDesc, s_pAllocator);

		if (pTexture == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create texture for \"%s\"", pName);
			return nullptr;
		}

		uint32 pixelDataSize = width * height * TextureFormatStride(format);

		BufferDesc bufferDesc	= {};
		bufferDesc.pName		= "Texture Copy Buffer";
		bufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
		bufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		bufferDesc.SizeInBytes	= pixelDataSize;

		IBuffer* pTextureData = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc, s_pAllocator);

		if (pTextureData == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to create copy buffer for \"%s\"", pName);
			return nullptr;
		}

		void* pTextureDataDst = pTextureData->Map();
		memcpy(pTextureDataDst, pData, pixelDataSize);
		pTextureData->Unmap();

		const uint64 waitValue = s_SignalValue - 1;
		s_pCopyFence->Wait(waitValue, UINT64_MAX);

		s_pCopyCommandAllocator->Reset();
		s_pCopyCommandList->Begin(nullptr);

		CopyTextureFromBufferDesc copyDesc = {};
		copyDesc.SrcOffset		= 0;
		copyDesc.SrcRowPitch	= 0;
		copyDesc.SrcHeight		= 0;
		copyDesc.Width			= width;
		copyDesc.Height			= height;
		copyDesc.Depth			= 1;
		copyDesc.Miplevel		= 0;
		copyDesc.MiplevelCount  = miplevels;
		copyDesc.ArrayIndex		= 0;
		copyDesc.ArrayCount		= 1;

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture					= pTexture;
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= textureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= textureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= textureDesc.ArrayCount;

		s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);

		s_pCopyCommandList->CopyTextureFromBuffer(pTextureData, pTexture, copyDesc);
		if (generateMips)
		{
			s_pCopyCommandList->GenerateMiplevels(pTexture, ETextureState::TEXTURE_STATE_COPY_DST, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
		}
		else
		{
			PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
			transitionToShaderReadBarrier.pTexture					= pTexture;
			transitionToShaderReadBarrier.StateBefore				= ETextureState::TEXTURE_STATE_COPY_DST;
			transitionToShaderReadBarrier.StateAfter				= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			transitionToShaderReadBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_NONE;
			transitionToShaderReadBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_NONE;
			transitionToShaderReadBarrier.SrcMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			transitionToShaderReadBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			transitionToShaderReadBarrier.TextureFlags				= textureDesc.Flags;
			transitionToShaderReadBarrier.Miplevel					= 0;
			transitionToShaderReadBarrier.MiplevelCount				= textureDesc.Miplevels;
			transitionToShaderReadBarrier.ArrayIndex				= 0;
			transitionToShaderReadBarrier.ArrayCount				= textureDesc.ArrayCount;

			s_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);
		}

		s_pCopyCommandList->End();

		if (!RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&s_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, s_pCopyFence, s_SignalValue))
		{
			LOG_ERROR("[ResourceLoader]: Texture could not be created as command list could not be executed for \"%s\"", pName);
			SAFERELEASE(pTextureData);
			return nullptr;
		}
		else
		{
			s_SignalValue++;
		}

		//Todo: Remove this wait after garbage collection works
		RenderSystem::GetGraphicsQueue()->Flush();
		SAFERELEASE(pTextureData);

		return pTexture;
	}

	IShader* ResourceLoader::LoadShaderFromFile(const char* pFilepath, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint)
	{
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		std::vector<uint32> sourceSPIRV;
		uint32 sourceSPIRVSize = 0;

		if (lang == EShaderLang::GLSL)
		{
			if (!ReadDataFromFile(pFilepath, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", pFilepath);
				return nullptr;
			}
			
			if (!CompileGLSLToSPIRV(pFilepath, reinterpret_cast<char*>(pShaderRawSource), shaderRawSourceSize, stage, sourceSPIRV))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", pFilepath);
				return nullptr;
			}

			sourceSPIRVSize = sourceSPIRV.size() * sizeof(uint32);
		}
		else if (lang == EShaderLang::SPIRV)
		{
			if (!ReadDataFromFile(pFilepath, "rb", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", pFilepath);
				return nullptr;
			}
			
			sourceSPIRV.resize((uint32)glm::ceil((float)shaderRawSourceSize / sizeof(uint32)));
			memcpy(sourceSPIRV.data(), pShaderRawSource, shaderRawSourceSize);

			sourceSPIRVSize = shaderRawSourceSize;
		}

		ShaderDesc shaderDesc = {};
		shaderDesc.pName				= pFilepath;
		shaderDesc.pSource				= reinterpret_cast<char*>(sourceSPIRV.data());
		shaderDesc.SourceSize			= sourceSPIRVSize;
		shaderDesc.pEntryPoint			= pEntryPoint;
		shaderDesc.Stage				= stage;
		shaderDesc.Lang					= lang;
		shaderDesc.pConstants			= nullptr;
		shaderDesc.ShaderConstantCount	= 0;

		IShader* pShader = RenderSystem::GetDevice()->CreateShader(&shaderDesc);

		SAFEDELETE_ARRAY(pShaderRawSource);

		return pShader;
	}

	ISoundEffect3D* ResourceLoader::LoadSoundEffectFromFile(const char* pFilepath)
	{
		SoundEffect3DDesc soundDesc = {};
		soundDesc.pFilepath = pFilepath;

		ISoundEffect3D* pSound = AudioSystem::GetDevice()->CreateSoundEffect(&soundDesc);

		if (pSound == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to initialize sound \"%s\"", pFilepath);
			return nullptr;
		}

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Sound \"%s\"", pFilepath);

		return pSound;
	}

	bool ResourceLoader::ReadDataFromFile(const char* pFilepath, const char* pMode, byte** ppData, uint32* pDataSize)
	{
		FILE* pFile = fopen(pFilepath, pMode);
		if (pFile == nullptr)
		{
			LOG_ERROR("[ResourceDevice]: Failed to load file \"%s\"", pFilepath);
			return false;
		}

		fseek(pFile, 0, SEEK_END);
		int32 length = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		byte* pData = (byte*)calloc(length, sizeof(byte));

		int32 read = fread(pData, 1, length, pFile);
		if (read == 0)
		{
			LOG_ERROR("[ResourceDevice]: Failed to read file \"%s\"", pFilepath);
			return false;
		}

		(*ppData)		= pData;
		(*pDataSize)	= length;

		fclose(pFile);
		return true;
	}

    void ResourceLoader::ConvertBackslashes(std::string& string)
    {
        size_t pos = string.find_first_of('\\');
        while (pos != std::string::npos)
        {
            string.replace(pos, 1, 1, '/');
            pos = string.find_first_of('\\', pos + 1);
        }
    }

	bool ResourceLoader::CompileGLSLToSPIRV(const char* pFilepath, const char* pSource, int32 sourceSize, FShaderStageFlags stage, std::vector<uint32>& sourceSPIRV)
	{
		static std::vector<uint32> RAYGEN_SPIRV =
		{
			0x07230203,0x00010000,0x00080008,0x00000076,0000000000,0x00020011,0x000014e9,0x0006000a,
			0x5f565053,0x5f52484b,0x5f796172,0x63617274,0x00676e69,0x0006000b,0x00000001,0x4c534c47,
			0x6474732e,0x3035342e,0000000000,0x0003000e,0000000000,0x00000001,0x0007000f,0x000014c1,
			0x00000004,0x6e69616d,0000000000,0x0000000d,0x00000017,0x00030003,0x00000002,0x000001cc,
			0x00060004,0x455f4c47,0x725f5458,0x745f7961,0x69636172,0x0000676e,0x00040005,0x00000004,
			0x6e69616d,0000000000,0x00050005,0x00000009,0x65786970,0x6e65436c,0x00726574,0x00060005,
			0x0000000d,0x4c5f6c67,0x636e7561,0x45444968,0x00005458,0x00040005,0x00000015,0x56556e69,
			0000000000,0x00070005,0x00000017,0x4c5f6c67,0x636e7561,0x7a695368,0x54584565,0000000000,
			0x00030005,0x0000001c,0x00000064,0x00040005,0x00000026,0x6a6f7270,0x00766e49,0x00040005,
			0x00000033,0x77656976,0x00766e49,0x00040005,0x0000003b,0x6769726f,0x00006e69,0x00040005,
			0x0000003f,0x67726174,0x00007465,0x00050005,0x0000004a,0x65726964,0x6f697463,0x0000006e,
			0x00040005,0x00000055,0x6e696d74,0000000000,0x00040005,0x00000057,0x78616d74,0000000000,
			0x00050005,0x0000005a,0x56746968,0x65756c61,0000000000,0x00050005,0x0000005e,0x4c706f74,
			0x6c657665,0x00005341,0x00040005,0x0000006b,0x67616d69,0x00000065,0x00040047,0x0000000d,
			0x0000000b,0x000014c7,0x00040047,0x00000017,0x0000000b,0x000014c8,0x00040047,0x0000005a,
			0x0000001e,0000000000,0x00040047,0x0000005e,0x00000022,0000000000,0x00040047,0x0000005e,
			0x00000021,0000000000,0x00040047,0x0000006b,0x00000022,0000000000,0x00040047,0x0000006b,
			0x00000021,0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
			0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,
			0x00000007,0x00000007,0x00040015,0x0000000a,0x00000020,0000000000,0x00040017,0x0000000b,
			0x0000000a,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,
			0x0000000d,0x00000001,0x00040017,0x0000000e,0x0000000a,0x00000002,0x0004002b,0x00000006,
			0x00000012,0x3f000000,0x0005002c,0x00000007,0x00000013,0x00000012,0x00000012,0x0004003b,
			0x0000000c,0x00000017,0x00000001,0x0004002b,0x00000006,0x0000001e,0x40000000,0x0004002b,
			0x00000006,0x00000020,0x3f800000,0x00040017,0x00000023,0x00000006,0x00000004,0x00040018,
			0x00000024,0x00000023,0x00000004,0x00040020,0x00000025,0x00000007,0x00000024,0x0004002b,
			0x00000006,0x00000027,0x3f836113,0x0004002b,0x00000006,0x00000028,0x80000000,0x0004002b,
			0x00000006,0x00000029,0000000000,0x0007002c,0x00000023,0x0000002a,0x00000027,0x00000028,
			0x00000029,0x00000028,0x0004002b,0x00000006,0x0000002b,0x3f13cd36,0x0007002c,0x00000023,
			0x0000002c,0x00000028,0x0000002b,0x00000028,0x00000029,0x0004002b,0x00000006,0x0000002d,
			0x411ff7ff,0x0007002c,0x00000023,0x0000002e,0x00000029,0x00000028,0x00000029,0x0000002d,
			0x0004002b,0x00000006,0x0000002f,0xbf800000,0x0004002b,0x00000006,0x00000030,0x3afffbce,
			0x0007002c,0x00000023,0x00000031,0x00000028,0x00000029,0x0000002f,0x00000030,0x0007002c,
			0x00000024,0x00000032,0x0000002a,0x0000002c,0x0000002e,0x00000031,0x0007002c,0x00000023,
			0x00000034,0x00000020,0x00000028,0x00000029,0x00000028,0x0007002c,0x00000023,0x00000035,
			0x00000028,0x00000020,0x00000028,0x00000029,0x0007002c,0x00000023,0x00000036,0x00000029,
			0x00000028,0x00000020,0x00000028,0x0004002b,0x00000006,0x00000037,0x40200000,0x0007002c,
			0x00000023,0x00000038,0x00000028,0x00000029,0x00000037,0x00000020,0x0007002c,0x00000024,
			0x00000039,0x00000034,0x00000035,0x00000036,0x00000038,0x00040020,0x0000003a,0x00000007,
			0x00000023,0x0007002c,0x00000023,0x0000003d,0x00000029,0x00000029,0x00000029,0x00000020,
			0x0004002b,0x0000000a,0x00000041,0000000000,0x00040020,0x00000042,0x00000007,0x00000006,
			0x0004002b,0x0000000a,0x00000045,0x00000001,0x00040017,0x0000004c,0x00000006,0x00000003,
			0x0004002b,0x00000006,0x00000056,0x3a83126f,0x0004002b,0x00000006,0x00000058,0x461c4000,
			0x00040020,0x00000059,0x000014da,0x0000004c,0x0004003b,0x00000059,0x0000005a,0x000014da,
			0x0006002c,0x0000004c,0x0000005b,0x00000029,0x00000029,0x00000029,0x000214dd,0x0000005c,
			0x00040020,0x0000005d,0000000000,0x0000005c,0x0004003b,0x0000005d,0x0000005e,0000000000,
			0x0004002b,0x0000000a,0x00000060,0x000000ff,0x00040015,0x00000067,0x00000020,0x00000001,
			0x0004002b,0x00000067,0x00000068,0000000000,0x00090019,0x00000069,0x00000006,0x00000001,
			0000000000,0000000000,0000000000,0x00000002,0x00000004,0x00040020,0x0000006a,0000000000,
			0x00000069,0x0004003b,0x0000006a,0x0000006b,0000000000,0x00040017,0x0000006f,0x00000067,
			0x00000002,0x00050036,0x00000002,0x00000004,0000000000,0x00000003,0x000200f8,0x00000005,
			0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003b,0x00000008,0x00000015,0x00000007,
			0x0004003b,0x00000008,0x0000001c,0x00000007,0x0004003b,0x00000025,0x00000026,0x00000007,
			0x0004003b,0x00000025,0x00000033,0x00000007,0x0004003b,0x0000003a,0x0000003b,0x00000007,
			0x0004003b,0x0000003a,0x0000003f,0x00000007,0x0004003b,0x0000003a,0x0000004a,0x00000007,
			0x0004003b,0x00000042,0x00000055,0x00000007,0x0004003b,0x00000042,0x00000057,0x00000007,
			0x0004003d,0x0000000b,0x0000000f,0x0000000d,0x0007004f,0x0000000e,0x00000010,0x0000000f,
			0x0000000f,0000000000,0x00000001,0x00040070,0x00000007,0x00000011,0x00000010,0x00050081,
			0x00000007,0x00000014,0x00000011,0x00000013,0x0003003e,0x00000009,0x00000014,0x0004003d,
			0x00000007,0x00000016,0x00000009,0x0004003d,0x0000000b,0x00000018,0x00000017,0x0007004f,
			0x0000000e,0x00000019,0x00000018,0x00000018,0000000000,0x00000001,0x00040070,0x00000007,
			0x0000001a,0x00000019,0x00050088,0x00000007,0x0000001b,0x00000016,0x0000001a,0x0003003e,
			0x00000015,0x0000001b,0x0004003d,0x00000007,0x0000001d,0x00000015,0x0005008e,0x00000007,
			0x0000001f,0x0000001d,0x0000001e,0x00050050,0x00000007,0x00000021,0x00000020,0x00000020,
			0x00050083,0x00000007,0x00000022,0x0000001f,0x00000021,0x0003003e,0x0000001c,0x00000022,
			0x0003003e,0x00000026,0x00000032,0x0003003e,0x00000033,0x00000039,0x0004003d,0x00000024,
			0x0000003c,0x00000033,0x00050091,0x00000023,0x0000003e,0x0000003c,0x0000003d,0x0003003e,
			0x0000003b,0x0000003e,0x0004003d,0x00000024,0x00000040,0x00000026,0x00050041,0x00000042,
			0x00000043,0x0000001c,0x00000041,0x0004003d,0x00000006,0x00000044,0x00000043,0x00050041,
			0x00000042,0x00000046,0x0000001c,0x00000045,0x0004003d,0x00000006,0x00000047,0x00000046,
			0x00070050,0x00000023,0x00000048,0x00000044,0x00000047,0x00000020,0x00000020,0x00050091,
			0x00000023,0x00000049,0x00000040,0x00000048,0x0003003e,0x0000003f,0x00000049,0x0004003d,
			0x00000024,0x0000004b,0x00000033,0x0004003d,0x00000023,0x0000004d,0x0000003f,0x0008004f,
			0x0000004c,0x0000004e,0x0000004d,0x0000004d,0000000000,0x00000001,0x00000002,0x0006000c,
			0x0000004c,0x0000004f,0x00000001,0x00000045,0x0000004e,0x00050051,0x00000006,0x00000050,
			0x0000004f,0000000000,0x00050051,0x00000006,0x00000051,0x0000004f,0x00000001,0x00050051,
			0x00000006,0x00000052,0x0000004f,0x00000002,0x00070050,0x00000023,0x00000053,0x00000050,
			0x00000051,0x00000052,0x00000029,0x00050091,0x00000023,0x00000054,0x0000004b,0x00000053,
			0x0003003e,0x0000004a,0x00000054,0x0003003e,0x00000055,0x00000056,0x0003003e,0x00000057,
			0x00000058,0x0003003e,0x0000005a,0x0000005b,0x0004003d,0x0000005c,0x0000005f,0x0000005e,
			0x0004003d,0x00000023,0x00000061,0x0000003b,0x0008004f,0x0000004c,0x00000062,0x00000061,
			0x00000061,0000000000,0x00000001,0x00000002,0x0004003d,0x00000006,0x00000063,0x00000055,
			0x0004003d,0x00000023,0x00000064,0x0000004a,0x0008004f,0x0000004c,0x00000065,0x00000064,
			0x00000064,0000000000,0x00000001,0x00000002,0x0004003d,0x00000006,0x00000066,0x00000057,
			0x000c14d9,0x0000005f,0x00000045,0x00000060,0x00000041,0x00000041,0x00000041,0x00000062,
			0x00000063,0x00000065,0x00000066,0x00000068,0x0004003d,0x00000069,0x0000006c,0x0000006b,
			0x0004003d,0x0000000b,0x0000006d,0x0000000d,0x0007004f,0x0000000e,0x0000006e,0x0000006d,
			0x0000006d,0000000000,0x00000001,0x0004007c,0x0000006f,0x00000070,0x0000006e,0x0004003d,
			0x0000004c,0x00000071,0x0000005a,0x00050051,0x00000006,0x00000072,0x00000071,0000000000,
			0x00050051,0x00000006,0x00000073,0x00000071,0x00000001,0x00050051,0x00000006,0x00000074,
			0x00000071,0x00000002,0x00070050,0x00000023,0x00000075,0x00000072,0x00000073,0x00000074,
			0x00000029,0x00040063,0x0000006c,0x00000070,0x00000075,0x000100fd,0x00010038
		};

		static std::vector<uint32> CLOSEST_HIT_SPIRV =
		{
			0x07230203,0x00010000,0x00080008,0x0000001f,0000000000,0x00020011,0x000014e9,0x0006000a,
			0x5f565053,0x5f52484b,0x5f796172,0x63617274,0x00676e69,0x0006000b,0x00000001,0x4c534c47,
			0x6474732e,0x3035342e,0000000000,0x0003000e,0000000000,0x00000001,0x0005000f,0x000014c4,
			0x00000004,0x6e69616d,0000000000,0x00030003,0x00000002,0x000001cc,0x00060004,0x455f4c47,
			0x725f5458,0x745f7961,0x69636172,0x0000676e,0x00040005,0x00000004,0x6e69616d,0000000000,
			0x00070005,0x00000009,0x79726162,0x746e6563,0x43636972,0x64726f6f,0x00000073,0x00040005,
			0x0000000c,0x72747461,0x00736269,0x00050005,0x0000001d,0x56746968,0x65756c61,0000000000,
			0x00040047,0x0000001d,0x0000001e,0000000000,0x00020013,0x00000002,0x00030021,0x00000003,
			0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
			0x00040020,0x00000008,0x00000007,0x00000007,0x0004002b,0x00000006,0x0000000a,0x3f800000,
			0x00040020,0x0000000b,0x000014db,0x00000007,0x0004003b,0x0000000b,0x0000000c,0x000014db,
			0x00040015,0x0000000d,0x00000020,0000000000,0x0004002b,0x0000000d,0x0000000e,0000000000,
			0x00040020,0x0000000f,0x000014db,0x00000006,0x0004002b,0x0000000d,0x00000013,0x00000001,
			0x00040020,0x0000001c,0x000014de,0x00000007,0x0004003b,0x0000001c,0x0000001d,0x000014de,
			0x00050036,0x00000002,0x00000004,0000000000,0x00000003,0x000200f8,0x00000005,0x0004003b,
			0x00000008,0x00000009,0x00000007,0x00050041,0x0000000f,0x00000010,0x0000000c,0x0000000e,
			0x0004003d,0x00000006,0x00000011,0x00000010,0x00050083,0x00000006,0x00000012,0x0000000a,
			0x00000011,0x00050041,0x0000000f,0x00000014,0x0000000c,0x00000013,0x0004003d,0x00000006,
			0x00000015,0x00000014,0x00050083,0x00000006,0x00000016,0x00000012,0x00000015,0x00050041,
			0x0000000f,0x00000017,0x0000000c,0x0000000e,0x0004003d,0x00000006,0x00000018,0x00000017,
			0x00050041,0x0000000f,0x00000019,0x0000000c,0x00000013,0x0004003d,0x00000006,0x0000001a,
			0x00000019,0x00060050,0x00000007,0x0000001b,0x00000016,0x00000018,0x0000001a,0x0003003e,
			0x00000009,0x0000001b,0x0004003d,0x00000007,0x0000001e,0x00000009,0x0003003e,0x0000001d,
			0x0000001e,0x000100fd,0x00010038
		};

		static std::vector<uint32> MISS_SPIRV =
		{
			0x07230203,0x00010000,0x00080008,0x0000000d,0000000000,0x00020011,0x000014e9,0x0006000a,
			0x5f565053,0x5f52484b,0x5f796172,0x63617274,0x00676e69,0x0006000b,0x00000001,0x4c534c47,
			0x6474732e,0x3035342e,0000000000,0x0003000e,0000000000,0x00000001,0x0005000f,0x000014c5,
			0x00000004,0x6e69616d,0000000000,0x00030003,0x00000002,0x000001cc,0x00060004,0x455f4c47,
			0x725f5458,0x745f7961,0x69636172,0x0000676e,0x00040005,0x00000004,0x6e69616d,0000000000,
			0x00050005,0x00000009,0x56746968,0x65756c61,0000000000,0x00040047,0x00000009,0x0000001e,
			0000000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
			0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,0x00040020,0x00000008,0x000014de,
			0x00000007,0x0004003b,0x00000008,0x00000009,0x000014de,0x0004002b,0x00000006,0x0000000a,
			0000000000,0x0004002b,0x00000006,0x0000000b,0x3e4ccccd,0x0006002c,0x00000007,0x0000000c,
			0x0000000a,0x0000000a,0x0000000b,0x00050036,0x00000002,0x00000004,0000000000,0x00000003,
			0x000200f8,0x00000005,0x0003003e,0x00000009,0x0000000c,0x000100fd,0x00010038
		};

		if (stage == FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER)
		{
			sourceSPIRV.resize(RAYGEN_SPIRV.size());
			memcpy(sourceSPIRV.data(), RAYGEN_SPIRV.data(), RAYGEN_SPIRV.size() * sizeof(uint32));
			return true;
		}
		else if (stage == FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER)
		{
			sourceSPIRV.resize(CLOSEST_HIT_SPIRV.size());
			memcpy(sourceSPIRV.data(), CLOSEST_HIT_SPIRV.data(), CLOSEST_HIT_SPIRV.size() * sizeof(uint32));
			return true;
		}
		else if (stage == FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER)
		{
			sourceSPIRV.resize(MISS_SPIRV.size());
			memcpy(sourceSPIRV.data(), MISS_SPIRV.data(), MISS_SPIRV.size() * sizeof(uint32));
			return true;
		}

		EShLanguage shaderType = ConvertShaderStageToEShLanguage(stage);
		glslang::TShader shader(shaderType);

		std::string source			= std::string(pSource);
		int32 foundBracket			= source.find_last_of('}') + 1;
		source[foundBracket]		= '\0';
		const char* pFinalSource	= source.c_str();
		shader.setStringsWithLengths(&pFinalSource, &foundBracket, 1);

		//Todo: Fetch this
		int32 clientInputSemanticsVersion							    = 110;
		glslang::EShTargetClientVersion vulkanClientVersion				= glslang::EShTargetVulkan_1_0;
		glslang::EShTargetLanguageVersion targetVersion					= glslang::EShTargetSpv_1_0;

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

		const TBuiltInResource* pResources	= GetDefaultBuiltInResources();
		EShMessages messages				= static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		const int defaultVersion			= 100;

		std::string preprocessedGLSL;
		DirStackFileIncluder includer;

		//Get Directory Path of File
		std::string filepath		= std::string(pFilepath);
		size_t found				= filepath.find_last_of("/\\");
		std::string directoryPath	= filepath.substr(0, found);

		includer.pushExternalLocalDirectory(directoryPath);

		if (!shader.preprocess(pResources, defaultVersion, ENoProfile, false, false, messages, &preprocessedGLSL, includer))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Preprocessing failed for: \"%s\"\n%s\n%s", pFilepath, shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		const char* pPreprocessedGLSL = preprocessedGLSL.c_str();
		shader.setStrings(&pPreprocessedGLSL, 1);

		if (!shader.parse(pResources, defaultVersion, false, messages))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Parsing failed for: \"%s\"\n%s\n%s", pFilepath, shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Linking failed for: \"%s\"\n%s\n%s", pFilepath, shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;
		glslang::GlslangToSpv(*program.getIntermediate(shaderType), sourceSPIRV, &logger, &spvOptions);
        
        return true;
	}
}
