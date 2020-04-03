#pragma once

#include "ResourceLoader.h"

#include <unordered_map>

namespace LambdaEngine
{
	class IGraphicsDevice;
	class AudioDevice;

	class LAMBDA_API ResourceManager
	{
		static constexpr GUID_Lambda DEFAULT_COLOR_MAP			= 0;
		static constexpr GUID_Lambda DEFAULT_NORMAL_MAP			= DEFAULT_COLOR_MAP		+ 1;
		static constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID	= DEFAULT_NORMAL_MAP	+ 1;

		static constexpr GUID_Lambda GUID_NONE = std::numeric_limits<GUID_Lambda>::max();

	public:
		DECL_REMOVE_COPY(ResourceManager);
		DECL_REMOVE_MOVE(ResourceManager);

		ResourceManager(IGraphicsDevice* pGraphicsDevice, AudioDevice* pAudioDevice);
		~ResourceManager();

		bool LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& result);

		GUID_Lambda LoadMeshFromFile(const char* pFilepath);
		GUID_Lambda LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		GUID_Lambda LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambienOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		GUID_Lambda LoadTextureFromFile(const char* pFilepath);
		GUID_Lambda LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips);

		GUID_Lambda LoadSoundFromFile(const char* pFilepath, ESoundFlags flags);

		Mesh*			GetMesh(GUID_Lambda guid)		{ return m_Meshes[guid]; }
		Material*		GetMaterial(GUID_Lambda guid)	{ return m_Materials[guid]; }
		ITexture*		GetTexture(GUID_Lambda guid)	{ return m_Textures[guid]; }
		SoundEffect3D*	GetSound(GUID_Lambda guid)		{ return m_Sounds[guid]; }

	private:
		GUID_Lambda RegisterLoadedMesh(Mesh* pMesh);
		GUID_Lambda RegisterLoadedMaterial(Material* pMaterial);
		GUID_Lambda RegisterLoadedTexture(ITexture* pTexture);

		void InitDefaultResources();

	private:
		IGraphicsDevice* m_pGraphicsDevice;
		AudioDevice* m_pAudioDevice;

		std::unordered_map<GUID_Lambda, Mesh*>	   m_Meshes;
		std::unordered_map<GUID_Lambda, Material*> m_Materials;
		std::unordered_map<GUID_Lambda, ITexture*> m_Textures;
		std::unordered_map<GUID_Lambda, SoundEffect3D*> m_Sounds;

	private:
		static GUID_Lambda s_NextFreeGUID;

	};
}