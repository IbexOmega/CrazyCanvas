#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class Texture;
	class TextureView;
	class Sampler;

	typedef uint32 FLoadedTextureFlags;
	enum FLoadedTextureFlag : FLoadedTextureFlags
	{
		LOADED_TEXTURE_FLAG_NONE				= 0,
		LOADED_TEXTURE_FLAG_ALBEDO				= FLAG(0),
		LOADED_TEXTURE_FLAG_NORMAL				= FLAG(1),
		LOADED_TEXTURE_FLAG_AO					= FLAG(2),
		LOADED_TEXTURE_FLAG_METALLIC			= FLAG(3),
		LOADED_TEXTURE_FLAG_ROUGHNESS			= FLAG(4),
		LOADED_TEXTURE_FLAG_METALLIC_ROUGHNESS	= FLAG(5),
	};

	struct LoadedTexture
	{
		Texture*				pTexture	= nullptr;
		FLoadedTextureFlags		Flags		= LOADED_TEXTURE_FLAG_NONE;
	};

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

	struct LoadedMaterial
	{
		MaterialProperties Properties;

		LoadedTexture* pAlbedoMap				= nullptr;
		LoadedTexture* pNormalMap				= nullptr;
		LoadedTexture* pAmbientOcclusionMap		= nullptr;
		LoadedTexture* pMetallicMap				= nullptr;
		LoadedTexture* pRoughnessMap			= nullptr;
		LoadedTexture* pMetallicRoughnessMap	= nullptr;

		TextureView* pAlbedoMapView					= nullptr;
		TextureView* pNormalMapView					= nullptr;
		TextureView* pAmbientOcclusionMapView		= nullptr;
		TextureView* pMetallicMapView				= nullptr;
		TextureView* pRoughnessMapView				= nullptr;
		TextureView* pMetallicRoughnessMapView		= nullptr;
	};

	struct Material
	{
		MaterialProperties Properties;

		Texture* pAlbedoMap					= nullptr;
		Texture* pNormalMap					= nullptr;
		Texture* pAOMetallicRoughnessMap	= nullptr;

		TextureView* pAlbedoMapView					= nullptr;
		TextureView* pNormalMapView					= nullptr;
		TextureView* pAOMetallicRoughnessMapView	= nullptr;
	};
}
