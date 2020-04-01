#include "Resources/ResourceHandler.h"

#include "Log/Log.h"

#include <tiny_obj_loader.h>
#include <tiny_obj_loader.cc>

#include <utility>

namespace LambdaEngine
{
	GUID_Lambda ResourceHandler::s_NextFreeGUID = ResourceHandler::SMALLEST_UNRESERVED_GUID;

	ResourceHandler::ResourceHandler(IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
		InitDefaultResources();
	}

	ResourceHandler::~ResourceHandler()
	{
	}

	void ResourceHandler::LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GraphicsObject>& result)
	{
	}

	GUID_Lambda ResourceHandler::LoadMeshFromFile(const char* pFilepath)
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

		GUID_Lambda guid = LoadMeshFromMemory(vertices.data(), vertices.size(), indices.data(), indices.size());

		return guid;
	}

	GUID_Lambda ResourceHandler::LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		GUID_Lambda guid = GUID_NONE;
		Mesh* pMappedMesh = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			pMappedMesh = &m_Meshes[guid]; //Creates new entry if not existing
		}

		pMappedMesh->pVertices		= new Vertex[numVertices];
		pMappedMesh->NumVertices	= numVertices;
		pMappedMesh->pIndices		= new uint32[numIndices];
		pMappedMesh->NumIndices		= numIndices;

		memcpy(pMappedMesh->pVertices, pVertices, numVertices * sizeof(Vertex));
		memcpy(pMappedMesh->pIndices, pIndices, numIndices * sizeof(uint32));

		return guid;
	}

	GUID_Lambda ResourceHandler::LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambienOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties)
	{
		GUID_Lambda guid = GUID_NONE;
		Material* pMappedMaterial = nullptr;

		//Spinlock
		{
			guid = s_NextFreeGUID++;
			pMappedMaterial = &m_Materials[guid]; //Creates new entry if not existing
		}

		ITexture* pAlbedoMap				= albedoMap				!= GUID_NONE ? m_Textures[albedoMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pNormalMap				= normalMap				!= GUID_NONE ? m_Textures[normalMap]			: m_Textures[DEFAULT_NORMAL_MAP];
		ITexture* pAmbientOcclusionMap		= ambienOcclusionMap	!= GUID_NONE ? m_Textures[ambienOcclusionMap]	: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pMetallicMap				= metallicMap			!= GUID_NONE ? m_Textures[metallicMap]			: m_Textures[DEFAULT_COLOR_MAP];
		ITexture* pRoughnessMap				= roughnessMap			!= GUID_NONE ? m_Textures[roughnessMap]			: m_Textures[DEFAULT_COLOR_MAP];
		
		pMappedMaterial->pAlbedoMap					= pAlbedoMap;
		pMappedMaterial->pNormalMap					= pNormalMap;
		pMappedMaterial->pAmbientOcclusionMap		= pAmbientOcclusionMap;
		pMappedMaterial->pMetallicMap				= pMetallicMap;
		pMappedMaterial->pRoughnessMap				= pRoughnessMap;
		pMappedMaterial->Properties					= properties;

		
		return guid;
	}

	GUID_Lambda ResourceHandler::LoadTextureFromFile(const char* pFilepath)
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromFile");
		return GUID_NONE;
	}

	GUID_Lambda ResourceHandler::LoadTextureFromMemory(const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips)
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function LoadTextureFromMemory");
		return GUID_NONE;
	}

	void ResourceHandler::InitDefaultResources()
	{
		LOG_WARNING("[ResourceDevice]: Call to unimplemented function InitDefaultResources");
	}
}