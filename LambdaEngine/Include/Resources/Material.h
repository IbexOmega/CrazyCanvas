#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class ITexture;
	class ITextureView;
	class ISampler;

	constexpr const float DEFAULT_EMISSIVE_EMISSION_STRENGTH = 250.0f;

	union MaterialProperties
	{
		struct
		{
			glm::vec4 Albedo;

			float Ambient;
			float Metallic;
			float Roughness;

			float EmissionStrength;
		};

		float Properties[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
	};

	struct Material
	{
		MaterialProperties Properties;

		ITexture* pAlbedoMap					= nullptr;
		ITexture* pNormalMap					= nullptr;
		ITexture* pAmbientOcclusionMap			= nullptr;
		ITexture* pMetallicMap					= nullptr;
		ITexture* pRoughnessMap					= nullptr;

		ITextureView* pAlbedoMapView			= nullptr;
		ITextureView* pNormalMapView			= nullptr;
		ITextureView* pAmbientOcclusionMapView	= nullptr;
		ITextureView* pMetallicMapView			= nullptr;
		ITextureView* pRoughnessMapView			= nullptr;
	};
}
