#pragma once

#include "Rendering/Core/API/IGraphicsDevice.h"

#include "Rendering/Core/API/ITexture.h"
#include "Mesh.h"
#include "Material.h"

namespace LambdaEngine
{
	class ResourceLoader
	{
	public:
		static void SetGraphicsDevice(IGraphicsDevice* pGraphicsDevice) { s_pGraphicsDevice = pGraphicsDevice; }

		static bool LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& loadedGraphicsObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures);

		static Mesh* LoadMeshFromFile(const char* pFilepath);
		static Mesh* LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		static ITexture* LoadTextureFromFile(const char* pFilepath);
		static ITexture* LoadTextureFromMemory(const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips);

	private:
		static IGraphicsDevice* s_pGraphicsDevice;

	};
}