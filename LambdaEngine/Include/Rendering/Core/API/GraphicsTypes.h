#pragma once
#include "LambdaEngine.h"

#include <vector>

namespace LambdaEngine
{
    enum class EMemoryType : uint8
    {
        NONE        = 0,
        CPU_MEMORY  = 1,
        GPU_MEMORY  = 2,
    };

    enum class EFormat : uint8
    {
        NONE            = 0,
        R8G8B8A8_UNORM  = 1,
        B8G8R8A8_UNORM  = 2,
    };

	enum class EShaderType : uint32
	{
		NONE				= 0,
		MESH_SHADER			= (1 << 0),
		VERTEX_SHADER		= (1 << 1),
		GEOMETRY_SHADER		= (1 << 2),
		HULL_SHADER			= (1 << 3),
		DOMAIN_SHADER		= (1 << 4),
		PIXEL_SHADER		= (1 << 5),
		COMPUTE_SHADER		= (1 << 6),
		RAYGEN_SHADER		= (1 << 7),
		INTERSECT_SHADER	= (1 << 8),
		ANY_HIT_SHADER		= (1 << 9),
		CLOSEST_HIT_SHADER	= (1 << 10),
		MISS_SHADER			= (1 << 11),
	};

	enum class EShaderLang : uint32
	{
		NONE				= 0,
		SPIRV				= (1 << 0),
	}; 

	union ShaderConstant
	{
		byte Data[4];
		float Float;
		int32 Integer;
	};

	struct ShaderDesc
	{
		const char* pSource = nullptr;
		uint32 SourceSize = 0;
		const char* pEntryPoint = "main";
		EShaderType Type = EShaderType::NONE;
		EShaderLang Lang = EShaderLang::NONE;
		std::vector<ShaderConstant> Constants;
	};
}
