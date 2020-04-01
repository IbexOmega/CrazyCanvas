#include "Resources/ResourceDevice.h"

#include "Log/Log.h"

#include <tiny_obj_loader.h>
#include <tiny_obj_loader.cc>

namespace LambdaEngine
{
	GUID_Lambda ResourceDevice::s_NextFreeGUID = ResourceDevice::SMALLEST_UNRESERVED_GUID;

	ResourceDevice::ResourceDevice()
	{
		InitDefaultResources();
	}

	ResourceDevice::~ResourceDevice()
	{
	}

	void ResourceDevice::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& result)
	{
	}

	GUID_Lambda ResourceDevice::LoadMeshFromFile(const char* pFilepath)
	{
		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, pFilepath, nullptr, true, false))
		{
			LOG_WARNING("[ResourceDevice]: Failed to load mesh '%s'. Warning: %s Error: %s", pFilepath, warn.c_str(), err.c_str());
			return GUID_NONE;
		}

		std::vector<Vertex> vertices = {};
		std::vector<uint32_t> indices = {};
		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const tinyobj::shape_t& shape : shapes)
		{
			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * index.vertex_index + 0],
					attributes.vertices[3 * index.vertex_index + 1],
					attributes.vertices[3 * index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					vertex.Normal =
					{
						attributes.normals[3 * index.normal_index + 0],
						attributes.normals[3 * index.normal_index + 1],
						attributes.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord =
					{
						attributes.texcoords[2 * index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		//Calculate tangents
		for (uint32_t index = 0; index < indices.size(); index += 3)
		{
			Vertex& v0 = vertices[indices[index + 0]];
			Vertex& v1 = vertices[indices[index + 1]];
			Vertex& v2 = vertices[indices[index + 2]];

			v0.CalculateTangent(v1, v2);
			v1.CalculateTangent(v2, v0);
			v2.CalculateTangent(v0, v1);
		}

		//TODO: Calculate normals

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Mesh \"%s\"", pFilepath);

		return LoadMeshFromMemory(vertices.data(), vertices.size(), indices.data(), indices.size());
	}

	GUID_Lambda ResourceDevice::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		Mesh mesh = {};
		mesh.pVertices		= new Vertex[numVertices];
		mesh.NumVertices	= numVertices;
		mesh.pIndices		= new uint32[numIndices];
		mesh.NumIndices		= numIndices;

		memcpy(mesh.pVertices, pVertices, numVertices * sizeof(Vertex));
		memcpy(mesh.pIndices, pIndices, numIndices * sizeof(uint32));

		//Spinlock
		{ 
			GUID_Lambda guid = s_NextFreeGUID++;
			m_Meshes[guid] = mesh;

			return guid;
		}
	}

	GUID_Lambda ResourceDevice::LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambienOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties)
	{
		ITexture* pAlbedoMap				= albedoMap				!= GUID_NONE ? m_Textures[albedoMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pNormalMap				= normalMap				!= GUID_NONE ? m_Textures[normalMap]			: m_Textures[DEFAULT_NORMAL_MAP];
		ITexture* pAmbientOcclusionMap		= ambienOcclusionMap	!= GUID_NONE ? m_Textures[ambienOcclusionMap]	: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pMetallicMap				= metallicMap			!= GUID_NONE ? m_Textures[metallicMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pRoughnessMap				= roughnessMap			!= GUID_NONE ? m_Textures[roughnessMap]			: m_Textures[DEFAULT_COLOR_MAP];
		
		Material material = {};
		material.pAlbedoMap					= pAlbedoMap;
		material.pNormalMap					= pNormalMap;
		material.pAmbientOcclusionMap		= pAmbientOcclusionMap;
		material.pMetallicMap				= pMetallicMap;
		material.pRoughnessMap				= pRoughnessMap;
		material.Properties					= properties;

		//Spinlock
		{
			GUID_Lambda guid = s_NextFreeGUID++;
			m_Materials[guid] = material;

			return guid;
		}
	}

	GUID_Lambda ResourceDevice::LoadTextureFromFile(const char* pFilepath)
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromFile");
		return GUID_NONE;
	}

	GUID_Lambda ResourceDevice::LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromMemory");
		return GUID_NONE;
	}

	void ResourceDevice::InitDefaultResources()
	{
	}

}