#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class Texture;
	class TextureView;
	class Sampler;

	constexpr const float DEFAULT_EMISSIVE_EMISSION_STRENGTH = 250.0f;

	union MaterialProperties
	{
		struct
		{
			glm::vec4 Albedo;

			float AO;
			float Metallic;
			float Roughness;

			float Unused;
		};

		float Properties[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
	};

	struct Material
	{
		MaterialProperties Properties;

		Texture* pAlbedoMap						= nullptr;
		Texture* pNormalMap						= nullptr;
		Texture* pAmbientOcclusionMap			= nullptr;
		Texture* pMetallicMap					= nullptr;
		Texture* pRoughnessMap					= nullptr;

		TextureView* pAlbedoMapView				= nullptr;
		TextureView* pNormalMapView				= nullptr;
		TextureView* pAmbientOcclusionMapView	= nullptr;
		TextureView* pMetallicMapView			= nullptr;
		TextureView* pRoughnessMapView			= nullptr;
	};
}
