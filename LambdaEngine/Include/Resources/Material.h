#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class ITexture;
	class ISampler;

	union MaterialProperties
	{
		struct
		{
			glm::vec4 Albedo;

			float Ambient;
			float Metallic;
			float Roughness;

			float Unreserved;
		};

		float Properties[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct Material
	{
		MaterialProperties Properties;

		ITexture* pAlbedoMap			= nullptr;
		ITexture* pNormalMap			= nullptr;
		ITexture* pAmbientOcclusionMap	= nullptr;
		ITexture* pMetallicMap			= nullptr;
		ITexture* pRoughnessMap			= nullptr;
	};
}
