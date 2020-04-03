#pragma once

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Audio/AudioDevice.h"

#include "Rendering/Core/API/ITexture.h"
#include "Mesh.h"
#include "Material.h"
#include "Audio/SoundEffect3D.h"

namespace LambdaEngine
{
	class ResourceLoader
	{
	public:
		static bool LoadSceneFromFile(IGraphicsDevice* pGraphicsDevice, const char* pDir, const char* pFilename, std::vector<GraphicsObject>& loadedGraphicsObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures);

		static Mesh* LoadMeshFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath);
		static Mesh* LoadMeshFromMemory(IGraphicsDevice* pGraphicsDevice, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		static ITexture* LoadTextureFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath);
		static ITexture* LoadTextureFromMemory(IGraphicsDevice* pGraphicsDevice, const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips);

		static SoundEffect3D* LoadSoundFromFile(AudioDevice* pAudioDevice, const char* pFilepath, ESoundFlags flags);

	private:
		static bool ReadDataFromFile(const char* pFilepath, byte** ppData, uint32* pDataSize);
	};
}