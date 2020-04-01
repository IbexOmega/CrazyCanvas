#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/Core/API/ITexture.h"
#include "Mesh.h"
#include "Material.h"

#include <unordered_map>

namespace LambdaEngine
{
	class ResourceDevice
	{
		static constexpr GUID_Lambda DEFAULT_COLOR_MAP			= 0;
		static constexpr GUID_Lambda DEFAULT_NORMAL_MAP		= DEFAULT_COLOR_MAP		+ 1;
		static constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID	= DEFAULT_NORMAL_MAP	+ 1;

		static constexpr GUID_Lambda GUID_NONE = std::numeric_limits<GUID_Lambda>::max();

	public:
		DECL_REMOVE_COPY(ResourceDevice);
		DECL_REMOVE_MOVE(ResourceDevice);

		ResourceDevice();
		~ResourceDevice();

		void LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& result);

		GUID_Lambda LoadMeshFromFile(const char* pFilepath);
		GUID_Lambda LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		GUID_Lambda LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambienOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		GUID_Lambda LoadTextureFromFile(const char* pFilepath);
		GUID_Lambda LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips);

		const Mesh*			GetMesh(GUID_Lambda guid)		{ return &m_Meshes[guid]; }
		const Material*		GetMaterial(GUID_Lambda guid)	{ return &m_Materials[guid]; }
		
		const ITexture*		GetTexture(GUID_Lambda guid)	{ return m_Textures[guid]; }

	private:
		void InitDefaultResources();

	private:
		//API Independent
		std::unordered_map<GUID_Lambda, Mesh> m_Meshes;
		std::unordered_map<GUID_Lambda, Material> m_Materials;

		//API Dependent
		std::unordered_map<GUID_Lambda, ITexture*> m_Textures;

	private:
		static GUID_Lambda s_NextFreeGUID;

	};
}