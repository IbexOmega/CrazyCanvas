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

#define MAX_PRIMS 126
#define MAX_VERTS 32

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
	static void ConvertBackslashes(String& string)
	{
		size_t pos = string.find_first_of('\\');
		while (pos != String::npos)
		{
			string.replace(pos, 1, 1, '/');
			pos = string.find_first_of('\\', pos + 1);
		}
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
	* SceneLoadingContext
	*/
	struct SceneLoadingContext
	{
		String Filepath;
		String DirectoryPath;
		TArray<Mesh*>		Meshes;
		TArray<Material*>	Materials;
		TArray<Texture*>	Textures;
		TArray<MeshComponent>	LoadedMeshComponent;
		THashTable<String, Texture*> LoadedTextures;
		THashTable<uint32, uint32> MaterialIndices;
	};

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
			ConvertBackslashes(name);
			RemoveExtraData(name);

			auto loadedTexture = context.LoadedTextures.find(name);
			if (loadedTexture == context.LoadedTextures.end())
			{
				Texture* pTexture = ResourceLoader::LoadTextureArrayFromFile(name, context.DirectoryPath, &name, 1, EFormat::FORMAT_R8G8B8A8_UNORM, true);
				context.LoadedTextures[name] = pTexture;
				return context.Textures.PushBack(pTexture);
			}
			else
			{
				return loadedTexture->second;
			}
		}

		return nullptr;
	}

	static void ProcessAssimpNode(SceneLoadingContext& context, const aiNode* pNode, const aiScene* pScene, bool createMaterials)
	{
		for (uint32 i = 0; i < pNode->mNumMeshes; i++)
		{
			aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];

			TArray<Vertex> vertices;
			for (uint32 j = 0; j < pMesh->mNumVertices; j++)
			{
				Vertex vertex;
				vertex.Position.x = pMesh->mVertices[j].x;
				vertex.Position.y = pMesh->mVertices[j].y;
				vertex.Position.z = pMesh->mVertices[j].z;

				if (pMesh->HasNormals())
				{
					vertex.Normal.x = pMesh->mNormals[j].x;
					vertex.Normal.y = pMesh->mNormals[j].y;
					vertex.Normal.z = pMesh->mNormals[j].z;
				}

				if (pMesh->HasTangentsAndBitangents())
				{
					vertex.Tangent.x = pMesh->mTangents[j].x;
					vertex.Tangent.y = pMesh->mTangents[j].y;
					vertex.Tangent.z = pMesh->mTangents[j].z;
				}

				if (pMesh->HasTextureCoords(0))
				{
					vertex.TexCoord.x = pMesh->mTextureCoords[0][j].x;
					vertex.TexCoord.y = pMesh->mTextureCoords[0][j].y;
				}

				vertices.PushBack(vertex);
			}

			VALIDATE(pMesh->HasFaces());

			TArray<uint32> indices;
			for (uint32 f = 0; f < pMesh->mNumFaces; f++)
			{
				aiFace face = pMesh->mFaces[f];
				for (uint32 j = 0; j < face.mNumIndices; j++)
				{
					indices.EmplaceBack(face.mIndices[j]);
				}
			}

			vertices.ShrinkToFit();
			indices.ShrinkToFit();

			if (createMaterials && pMesh->mMaterialIndex >= 0)
			{
				auto mat = context.MaterialIndices.find(pMesh->mMaterialIndex);
				if (mat == context.MaterialIndices.end())
				{
					Material*	pMaterial	= DBG_NEW Material();
					aiMaterial* pAiMaterial	= pScene->mMaterials[pMesh->mMaterialIndex];
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
					if (aiGetMaterialColor(pAiMaterial, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS)
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
					pMaterial->pAlbedoMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_BASE_COLOR, 0);
					if (!pMaterial->pAlbedoMap)
					{
						pMaterial->pAlbedoMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_DIFFUSE, 0);
					}

					// Normal
					pMaterial->pNormalMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_NORMAL_CAMERA, 0);
					if (!pMaterial->pNormalMap)
					{
						pMaterial->pNormalMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_NORMALS, 0);
					}
					if (!pMaterial->pNormalMap)
					{
						pMaterial->pNormalMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_HEIGHT, 0);
					}

					// AO
					pMaterial->pAmbientOcclusionMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_AMBIENT_OCCLUSION, 0);
					if (!pMaterial->pAmbientOcclusionMap)
					{
						pMaterial->pAmbientOcclusionMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_AMBIENT, 0);
					}

					// Metallic
					pMaterial->pMetallicMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_METALNESS, 0);
					if (!pMaterial->pMetallicMap)
					{
						pMaterial->pMetallicMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_REFLECTION, 0);
					}

					// Roughness
					pMaterial->pRoughnessMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_DIFFUSE_ROUGHNESS, 0);
					if (!pMaterial->pRoughnessMap)
					{
						pMaterial->pRoughnessMap = LoadAssimpTexture(context, pAiMaterial, aiTextureType_SHININESS, 0);
					}

					context.Materials.EmplaceBack(pMaterial);
					context.MaterialIndices[pMesh->mMaterialIndex] = context.Materials.GetSize() - 1;
				}
			}

			Mesh* pNewMesh = ResourceLoader::LoadMeshFromMemory(vertices.GetData(), vertices.GetSize(), indices.GetData(), indices.GetSize());
			if (pNewMesh)
			{
				context.Meshes.EmplaceBack(pNewMesh);

				MeshComponent newMeshComponent;
				newMeshComponent.MeshGUID		= context.Meshes.GetSize() - 1;
				newMeshComponent.MaterialGUID	= context.MaterialIndices[pMesh->mMaterialIndex];
				context.LoadedMeshComponent.PushBack(newMeshComponent);
			}

		}

		for (uint32 i = 0; i < pNode->mNumChildren; i++)
		{
			ProcessAssimpNode(context, pNode->mChildren[i], pScene, createMaterials);
		}
	}

	bool ResourceLoader::LoadSceneFromFile(const String& filepath, TArray<MeshComponent>& loadedMeshComponents, TArray<Mesh*>& loadedMeshes, TArray<Material*>& loadedMaterials, TArray<Texture*>& loadedTextures)
	{
		size_t lastPathDivisor = filepath.find_last_of("/\\");
		if (lastPathDivisor == String::npos)
		{
			LOG_WARNING("[ResourceLoader]: Failed to load scene '%s'. No parent directory found...", filepath.c_str());
			return false;
		}

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

		Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(filepath, assimpFlags);
		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load scene '%s'. Error: %s", filepath.c_str(), importer.GetErrorString());
			return false;
		}

		VALIDATE(pScene != nullptr);

		SceneLoadingContext context;
		context.Filepath		= filepath;
		context.DirectoryPath	= filepath.substr(0, lastPathDivisor + 1);
		ProcessAssimpNode(context, pScene->mRootNode, pScene, true);

		loadedMaterials			= Move(context.Materials);
		loadedTextures			= Move(context.Textures);
		loadedMeshComponents	= Move(context.LoadedMeshComponent);
		loadedMeshes			= Move(context.Meshes);

		return true;
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

		Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(filepath, assimpFlags);
		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load scene '%s'. Error: %s", filepath.c_str(), importer.GetErrorString());
			return false;
		}

		VALIDATE(pScene != nullptr);

		SceneLoadingContext context;
		context.Filepath = filepath;
		ProcessAssimpNode(context, pScene->mRootNode, pScene, false);

		D_LOG_MESSAGE("[ResourceLoader]: Loaded Mesh \"%s\"", filepath.c_str());
		return context.Meshes.GetFront();
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

		GenerateMeshlets(pMesh, MAX_VERTS, MAX_PRIMS);
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
			String filepath = dir + pFilenames[i];

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
			String filepath = dir + pFilenames[i];

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
		textureDesc.Flags		= FTextureFlag::TEXTURE_FLAG_COPY_SRC | FTextureFlag::TEXTURE_FLAG_COPY_DST | usageFlags;
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
		byte* pShaderRawSource = nullptr;
		uint32 shaderRawSourceSize = 0;

		TArray<uint32> sourceSPIRV;
		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(filepath, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return nullptr;
			}
			
			if (!CompileGLSLToSPIRV(filepath, reinterpret_cast<char*>(pShaderRawSource), stage, &sourceSPIRV, nullptr))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", filepath.c_str());
				return nullptr;
			}
		}
		else if (lang == EShaderLang::SHADER_LANG_SPIRV)
		{
			if (!ReadDataFromFile(filepath, "rb", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return nullptr;
			}
			
			sourceSPIRV.Resize(static_cast<uint32>(glm::ceil(static_cast<float32>(shaderRawSourceSize) / sizeof(uint32))));
			memcpy(sourceSPIRV.GetData(), pShaderRawSource, shaderRawSourceSize);
		}

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= filepath;
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

		std::vector<uint32> sourceSPIRV;
		uint32 sourceSPIRVSize = 0;

		if (lang == EShaderLang::SHADER_LANG_GLSL)
		{
			if (!ReadDataFromFile(filepath, "r", &pShaderRawSource, &shaderRawSourceSize))
			{
				LOG_ERROR("[ResourceLoader]: Failed to open shader file \"%s\"", filepath.c_str());
				return false;
			}

			if (!CompileGLSLToSPIRV(filepath, reinterpret_cast<char*>(pShaderRawSource), stage, nullptr, pReflection))
			{
				LOG_ERROR("[ResourceLoader]: Failed to compile GLSL to SPIRV for \"%s\"", filepath.c_str());
				return false;
			}

			sourceSPIRVSize = uint32(sourceSPIRV.size() * sizeof(uint32));
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
		soundDesc.Filepath = filepath;

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
		FILE* pFile = fopen(filepath.c_str(), pMode);
		if (pFile == nullptr)
		{
			LOG_ERROR("[ResourceLoader]: Failed to load file \"%s\"", filepath.c_str());
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
			LOG_ERROR("[ResourceLoader]: Failed to read file \"%s\"", filepath.c_str());
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
		int32 clientInputSemanticsVersion							    = 100;
		glslang::EShTargetClientVersion vulkanClientVersion				= glslang::EShTargetVulkan_1_2;
		glslang::EShTargetLanguageVersion targetVersion					= glslang::EShTargetSpv_1_5;

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

		const TBuiltInResource* pResources	= GetDefaultBuiltInResources();
		EShMessages messages				= static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		const int defaultVersion			= 110;

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
			LOG_ERROR("[ResourceLoader]: GLSL Parsing failed for: \"%s\"\n%s\n%s", filepath.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
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

	static glm::vec3 ComputeNormal(glm::vec3 positions[3])
	{
		glm::vec3 e0 = positions[0] - positions[1];
		glm::vec3 e1 = positions[1] - positions[2];
		return glm::normalize(glm::cross(e0, e1));
	}

	static void GenerateAdjecenyList(Mesh* pMesh, uint32* pAdjecency)
	{
		const uint32 indexCount = pMesh->IndexCount;
		const uint32 vertexCount = pMesh->VertexCount;
		const uint32 triangleCount = (indexCount / 3);

		const Mesh::IndexType* pIndices = pMesh->pIndexArray;
		const Vertex* pVertices = pMesh->pVertexArray;

		TArray<Mesh::IndexType> indexList(vertexCount);

		std::unordered_map<size_t, Mesh::IndexType> uniquePositions;
		uniquePositions.reserve(vertexCount);

		std::hash<glm::vec3> hasher;
		for (uint32 i = 0; i < vertexCount; i++)
		{
			size_t hash = hasher(pVertices[i].Position);

			auto it = uniquePositions.find(hash);
			if (it != uniquePositions.end())
			{
				indexList[i] = it->second;
			}
			else
			{
				uniquePositions.insert(std::make_pair(hash, static_cast<Mesh::IndexType>(i)));
				indexList[i] = static_cast<Mesh::IndexType>(i);
			}
		}

		const uint32 hashSize = vertexCount / 3;
		TUniquePtr<EdgeEntry[]> entries(DBG_NEW EdgeEntry[triangleCount * 3]);
		TUniquePtr<EdgeEntry*[]> hashTable(DBG_NEW EdgeEntry * [hashSize]);
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

						float32 dot = glm::dot(n0, n1);
						if (dot > bestDot)
						{
							pFound = pCurrent;
							pFoundPrev = pPrev;
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

					pAdjecency[index + point] = pFound->Face;

					uint32 key2 = i1 % hashSize;
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
							pAdjecency[(pFound->Face * 3) + edge2] = face;
						}
					}
				}
			}
		}
	}

	static bool AddToMeshlet(uint32 maxVerts, uint32 maxPrims, InlineMeshlet& meshlet, uint32(&tri)[3])
	{
		if (meshlet.UniqueVertexIndices.GetSize() == maxVerts)
		{
			return false;
		}

		if (meshlet.PrimitiveIndices.GetSize() == maxPrims)
		{
			return false;
		}

		static const uint32 undef = static_cast<uint32>(-1);
		uint32 indices[3] = { undef, undef, undef };
		uint32 newCount = 3;

		for (uint32 i = 0; i < meshlet.UniqueVertexIndices.GetSize(); i++)
		{
			for (uint32 j = 0; j < 3; j++)
			{
				if (meshlet.UniqueVertexIndices[i] == tri[j])
				{
					indices[j] = i;
					--newCount;
				}
			}
		}

		if (meshlet.UniqueVertexIndices.GetSize() + newCount > maxVerts)
		{
			return false;
		}

		for (uint32 i = 0; i < 3; i++)
		{
			if (indices[i] == undef)
			{
				indices[i] = static_cast<uint32>(meshlet.UniqueVertexIndices.GetSize());
				meshlet.UniqueVertexIndices.PushBack(tri[i]);
			}
		}

		InlineMeshlet::PackedTriangle prim = { };
		prim.i0 = indices[0];
		prim.i1 = indices[1];
		prim.i2 = indices[2];
		meshlet.PrimitiveIndices.PushBack(prim);

		return true;
	}

	static bool IsMeshletFull(uint32_t maxVerts, uint32_t maxPrims, const InlineMeshlet& meshlet)
	{
		VALIDATE(meshlet.UniqueVertexIndices.GetSize() <= maxVerts);
		VALIDATE(meshlet.PrimitiveIndices.GetSize() <= maxPrims);

		return meshlet.UniqueVertexIndices.GetSize() == maxVerts || meshlet.PrimitiveIndices.GetSize() == maxPrims;
	}

	static uint32 ComputeMeshletReuse(InlineMeshlet& meshlet, uint32(&triIndices)[3])
	{
		uint32 count = 0;
		for (uint32 i = 0; i < static_cast<uint32_t>(meshlet.UniqueVertexIndices.GetSize()); i++)
		{
			for (uint32 j = 0; j < 3; j++)
			{
				if (meshlet.UniqueVertexIndices[i] == triIndices[j])
				{
					count++;
				}
			}
		}

		return count;
	}

	static float32 ComputeMeshletScore(InlineMeshlet& meshlet, glm::vec3 normal, uint32(&triIndices)[3], glm::vec3* pTriVerts)
	{
		const float32 reuseWeight = 0.5f;
		const float32 oriWeight = 0.5f;

		const uint32 reuse = ComputeMeshletReuse(meshlet, triIndices);
		float32 reuseScore = float32(reuse) / 3.0f;

		glm::vec3 n = ComputeNormal(pTriVerts);
		float32 dot = glm::dot(n, normal);
		float32 oriScore = (-dot + 1.0f) / 2.0f;

		return (reuseWeight * reuseScore) + (oriWeight * oriScore);
	}

	static void Meshletize(Mesh* pMesh, uint32 maxVerts, uint32 maxPrims, TArray<InlineMeshlet>& output)
	{
		Mesh::IndexType* pIndices = pMesh->pIndexArray;
		Vertex* pVertices = pMesh->pVertexArray;

		const uint32 vertexCount = pMesh->VertexCount;
		const uint32 indexCount = pMesh->IndexCount;
		const uint32 triangleCount = (indexCount / 3);

		TArray<uint32> adjecenyList(indexCount);
		GenerateAdjecenyList(pMesh, adjecenyList.GetData());
		adjecenyList.ShrinkToFit();

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

			if (AddToMeshlet(maxVerts, maxPrims, *pCurr, tri))
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
						pVertices[triIndices[0]].Position,
						pVertices[triIndices[1]].Position,
						pVertices[triIndices[2]].Position,
					};

					candidates[i].second = ComputeMeshletScore(*pCurr, normal, triIndices, triVerts);
				}

				if (IsMeshletFull(maxVerts, maxPrims, *pCurr))
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
					pCurr->PrimitiveIndices.ShrinkToFit();
					pCurr->UniqueVertexIndices.ShrinkToFit();
					pCurr = &output.GetBack();
				}
				else
				{
					std::sort(candidates.begin(), candidates.end(), [](const std::pair<uint32, float32>& a, const std::pair<uint32, float32>& b)
						{
							return a.second > b.second;
						});
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
					pCurr->PrimitiveIndices.ShrinkToFit();
					pCurr->UniqueVertexIndices.ShrinkToFit();
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

		if (output.GetBack().PrimitiveIndices.IsEmpty())
		{
			output.PopBack();
		}
		else
		{
			output.GetBack().PrimitiveIndices.ShrinkToFit();
			output.GetBack().UniqueVertexIndices.ShrinkToFit();
		}
	}

	void ResourceLoader::GenerateMeshlets(Mesh* pMesh, uint32 maxVerts, uint32 maxPrims)
	{
		VALIDATE(pMesh->pIndexArray != nullptr);
		VALIDATE(pMesh->pVertexArray != nullptr);

		TArray<InlineMeshlet> builtMeshlets;
		Meshletize(pMesh, maxVerts, maxPrims, builtMeshlets);

		uint32 uniqueVertexIndexCount = 0;
		uint32 primitiveIndexCount = 0;
		uint32 meshletCount = static_cast<uint32>(builtMeshlets.GetSize());
		pMesh->MeshletCount = meshletCount;
		pMesh->pMeshletArray = DBG_NEW Meshlet[meshletCount];
		for (uint32 i = 0; i < meshletCount; i++)
		{
			pMesh->pMeshletArray[i].VertOffset = uniqueVertexIndexCount;
			pMesh->pMeshletArray[i].VertCount = static_cast<uint32>(builtMeshlets[i].UniqueVertexIndices.GetSize());
			uniqueVertexIndexCount += static_cast<uint32>(builtMeshlets[i].UniqueVertexIndices.GetSize());

			pMesh->pMeshletArray[i].VertOffset = primitiveIndexCount;
			pMesh->pMeshletArray[i].VertCount = static_cast<uint32>(builtMeshlets[i].PrimitiveIndices.GetSize());
			primitiveIndexCount += static_cast<uint32>(builtMeshlets[i].PrimitiveIndices.GetSize());
		}

		pMesh->PrimitiveIndexCount = primitiveIndexCount;
		pMesh->pPrimitiveIndices = DBG_NEW Mesh::IndexType[primitiveIndexCount];
		pMesh->UniqueIndexCount = uniqueVertexIndexCount;
		pMesh->pUniqueIndices = DBG_NEW Mesh::IndexType[uniqueVertexIndexCount];

		Mesh::IndexType* pUniqueIndices = pMesh->pUniqueIndices;
		Mesh::IndexType* pPrimitiveIndices = pMesh->pPrimitiveIndices;
		for (uint32 i = 0; i < meshletCount; i++)
		{
			uint32 localPrimitiveIndexCount = builtMeshlets[i].PrimitiveIndices.GetSize();
			uint32 localUniqueVertexIndexCount = builtMeshlets[i].UniqueVertexIndices.GetSize();
			memcpy(pPrimitiveIndices, builtMeshlets[i].PrimitiveIndices.GetData(), sizeof(Mesh::IndexType) * localPrimitiveIndexCount);
			memcpy(pUniqueIndices, builtMeshlets[i].UniqueVertexIndices.GetData(), sizeof(Mesh::IndexType) * localUniqueVertexIndexCount);
			pPrimitiveIndices += localPrimitiveIndexCount;
			pUniqueIndices += localUniqueVertexIndexCount;
		}
	}
}
