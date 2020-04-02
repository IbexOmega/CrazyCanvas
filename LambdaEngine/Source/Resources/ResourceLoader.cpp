#include "Resources/ResourceLoader.h"
#include "Log/Log.h"

#include <tiny_obj_loader.h>
#include <tiny_obj_loader.cc>

#include <cstdio>
#include <unordered_map>

namespace LambdaEngine
{
	bool ResourceLoader::LoadSceneFromFile(IGraphicsDevice* pGraphicsDevice, const char* pDir, const char* pFilename, std::vector<GraphicsObject>& loadedGraphicsObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures)
	{
		std::string filepath = std::string(pDir) + std::string(pFilename);

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filepath.c_str(), pDir, true, false))
		{
			LOG_WARNING("[ResourceDevice]: Failed to load scene '%s'. Warning: %s Error: %s", filepath, warn.c_str(), err.c_str());
			return false;
		}

		loadedMeshes.resize(shapes.size());
		loadedMaterials.resize(materials.size());

		std::unordered_map<std::string, ITexture*> loadedTexturesMap;

		for (uint32 m = 0; m < materials.size(); m++)
		{
			tinyobj::material_t& material = materials[m];

			Material* pMaterial = new Material();


			if (material.diffuse_texname.length() > 0)
			{
				std::string texturePath = pDir + material.diffuse_texname;
				auto loadedTexture = loadedTexturesMap.find(texturePath);

				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(pGraphicsDevice, texturePath.c_str());
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pAlbedoMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pAlbedoMap = loadedTexture->second;
				}
			}

			if (material.bump_texname.length() > 0)
			{
				std::string texturePath = pDir + material.bump_texname;
				auto loadedTexture = loadedTexturesMap.find(texturePath);

				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(pGraphicsDevice, texturePath.c_str());
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pNormalMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pNormalMap = loadedTexture->second;
				}
			}

			if (material.ambient_texname.length() > 0)
			{
				std::string texturePath = pDir + material.ambient_texname;
				auto loadedTexture = loadedTexturesMap.find(texturePath);

				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(pGraphicsDevice, texturePath.c_str());
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pMetallicMap			= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pMetallicMap = loadedTexture->second;
				}
			}

			if (material.specular_highlight_texname.length() > 0)
			{
				std::string texturePath = pDir + material.specular_highlight_texname;
				auto loadedTexture = loadedTexturesMap.find(texturePath);

				if (loadedTexture == loadedTexturesMap.end())
				{
					ITexture* pTexture = LoadTextureFromFile(pGraphicsDevice, texturePath.c_str());
					loadedTexturesMap[texturePath]	= pTexture;
					pMaterial->pRoughnessMap		= pTexture;

					loadedTextures.push_back(pTexture);
				}
				else
				{
					pMaterial->pRoughnessMap = loadedTexture->second;
				}
			}

			loadedMaterials[m] = pMaterial;
		}

		glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.005f));
		for (uint32 s = 0; s < shapes.size(); s++)
		{
			tinyobj::shape_t& shape = shapes[s];

			std::vector<Vertex> vertices = {};
			std::vector<uint32> indices = {};
			std::unordered_map<Vertex, uint32> uniqueVertices = {};

			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				//Normals and texcoords are optional, while positions are required
				ASSERT(index.vertex_index >= 0);

				vertex.Position =
				{
					attributes.vertices[3 * (size_t)index.vertex_index + 0],
					attributes.vertices[3 * (size_t)index.vertex_index + 1],
					attributes.vertices[3 * (size_t)index.vertex_index + 2]
				};

				if (index.normal_index >= 0)
				{
					vertex.Normal =
					{
						attributes.normals[3 * (size_t)index.normal_index + 0],
						attributes.normals[3 * (size_t)index.normal_index + 1],
						attributes.normals[3 * (size_t)index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.TexCoord =
					{
						attributes.texcoords[2 * (size_t)index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * (size_t)index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}

			//Calculate tangents
			for (uint32 index = 0; index < indices.size(); index += 3)
			{
				Vertex& v0 = vertices[indices[(size_t)index + 0]];
				Vertex& v1 = vertices[indices[(size_t)index + 1]];
				Vertex& v2 = vertices[indices[(size_t)index + 2]];

				v0.CalculateTangent(v1, v2);
				v1.CalculateTangent(v2, v0);
				v2.CalculateTangent(v0, v1);
			}

			Mesh* pMesh = LoadMeshFromMemory(pGraphicsDevice, vertices.data(), vertices.size(), indices.data(), indices.size());
			loadedMeshes[s] = pMesh;

			D_LOG_MESSAGE("[ResourceDevice]: Loaded Mesh \"%s\" \t for scene : \"%s\"", shape.name.c_str(), pFilename);

			uint32 m = shape.mesh.material_ids[0];

			GraphicsObject graphicsObject	= {};
			graphicsObject.Mesh			= s;
			graphicsObject.Material		= m;

			loadedGraphicsObjects.push_back(graphicsObject);
		}

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Scene \"%s\"", pFilename);

		return true;
	}

	Mesh* ResourceLoader::LoadMeshFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath)
	{
		//Start New Thread

		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, pFilepath, nullptr, true, false))
		{
			LOG_WARNING("[ResourceDevice]: Failed to load mesh '%s'. Warning: %s Error: %s", pFilepath, warn.c_str(), err.c_str());
			return nullptr;
		}

		std::vector<Vertex> vertices = {};
		std::vector<uint32> indices = {};
		std::unordered_map<Vertex, uint32> uniqueVertices = {};

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
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		//Calculate tangents
		for (uint32 index = 0; index < indices.size(); index += 3)
		{
			Vertex& v0 = vertices[indices[index + 0]];
			Vertex& v1 = vertices[indices[index + 1]];
			Vertex& v2 = vertices[indices[index + 2]];

			v0.CalculateTangent(v1, v2);
			v1.CalculateTangent(v2, v0);
			v2.CalculateTangent(v0, v1);
		}

		Mesh* pMesh = LoadMeshFromMemory(pGraphicsDevice, vertices.data(), vertices.size(), indices.data(), indices.size());

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Mesh \"%s\"", pFilepath);

		return pMesh;
	}

	Mesh* ResourceLoader::LoadMeshFromMemory(IGraphicsDevice* pGraphicsDevice, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices)
	{
		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.pName			= "Vertex Buffer (ResourceLoader)";
		vertexBufferDesc.MemoryType		= EMemoryType::GPU_MEMORY;
		vertexBufferDesc.SizeInBytes	= sizeof(Vertex) * numVertices;
		vertexBufferDesc.Flags			= EBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;// | EBufferFlags::BUFFER_FLAG_COPY_DST | EBufferFlags::BUFFER_FLAG_COPY_SRC; Maybe need these later?

		IBuffer* pVertexBuffer = pGraphicsDevice->CreateBuffer(vertexBufferDesc);

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.pName			= "Index Buffer (ResourceLoader)";
		indexBufferDesc.MemoryType		= EMemoryType::GPU_MEMORY;
		indexBufferDesc.SizeInBytes		= sizeof(uint32) * numIndices;
		indexBufferDesc.Flags			= EBufferFlags::BUFFER_FLAG_INDEX_BUFFER;// | EBufferFlags::BUFFER_FLAG_COPY_DST | EBufferFlags::BUFFER_FLAG_COPY_SRC; Maybe need these later?

		IBuffer* pIndexBuffer = pGraphicsDevice->CreateBuffer(indexBufferDesc);

		Mesh* pMesh = new Mesh();
		pMesh->pVertexBuffer	= pVertexBuffer;
		pMesh->VertexCount		= numVertices;
		pMesh->pIndexBuffer		= pIndexBuffer;
		pMesh->IndexCount		= numIndices;

		return pMesh;
	}

	ITexture* ResourceLoader::LoadTextureFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath)
	{
		LOG_WARNING("[ResourceLoader]: Call to unimplemented function LoadTextureFromFile");
		return nullptr;
	}

	ITexture* ResourceLoader::LoadTextureFromMemory(IGraphicsDevice* pGraphicsDevice, const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips)
	{
		LOG_WARNING("[ResourceLoader]: Call to unimplemented function LoadTextureFromMemory");
		return nullptr;
	}

	Sound* ResourceLoader::LoadSoundFromFile(AudioDevice* pAudioDevice, const char* pFilepath, ESoundFlags flags)
	{
		Sound* pSound = pAudioDevice->CreateSound();

		byte* pSoundData = nullptr;
		uint32 soundDataSize = 0;

		if (!ReadDataFromFile(pFilepath, &pSoundData, &soundDataSize))
		{
			return nullptr;
		}

		SoundDesc soundDesc		= {};
		soundDesc.pName			= pFilepath;
		soundDesc.pData			= pSoundData;
		soundDesc.DataSize		= soundDataSize;
		soundDesc.Flags			= flags;

		if (!pSound->Init(soundDesc))
		{
			LOG_WARNING("[ResourceDevice]: Failed to initialize sound \"%s\"", pFilepath);
			return nullptr;
		}

		SAFEDELETEARR(pSoundData);

		D_LOG_MESSAGE("[ResourceDevice]: Loaded Sound \"%s\"", pFilepath);

		return pSound;
	}

	bool ResourceLoader::ReadDataFromFile(const char* pFilepath, byte** ppData, uint32* pDataSize)
	{
		FILE* pFile = fopen(pFilepath, "r");

		if (pFile == nullptr)
		{
			LOG_WARNING("[ResourceDevice]: Failed to load file \"%s\"", pFilepath);
			return false;
		}

		fseek(pFile, 0, SEEK_END);
		(*pDataSize) = ftell(pFile);
		rewind(pFile);

		(*ppData) = new byte[(*pDataSize)];

		fread(*ppData, 1, (*pDataSize), pFile);

		fclose(pFile);

		return true;
	}
}