#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"

#include <glslangStandAlone/DirStackFileIncluder.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/MachineIndependent/reflection.h>

namespace LambdaEngine
{
	/*
	*  --------------------------glslang Helpers Begin---------------------------------
	*/

	FORCEINLINE const TBuiltInResource* GetDefaultBuiltInResources()
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

	FORCEINLINE int32 GetDefaultClientInputSemanticsVersion()
	{
		return 100;
	}

	FORCEINLINE glslang::EShTargetClientVersion GetDefaultVulkanClientVersion()
	{
		return glslang::EShTargetVulkan_1_2;
	}

	FORCEINLINE glslang::EShTargetLanguageVersion GetDefaultSPIRVTargetVersion()
	{
		return glslang::EShTargetSpv_1_5;
	}

	FORCEINLINE EShMessages GetDefaultMessages()
	{
		return static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
	}

	FORCEINLINE int32 GetDefaultVersion()
	{
		return 450;
	}

	FORCEINLINE EShLanguage ConvertShaderStageToEShLanguage(FShaderStageFlags shaderStage)
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
}