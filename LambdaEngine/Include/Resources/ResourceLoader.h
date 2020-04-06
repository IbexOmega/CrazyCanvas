#pragma once

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Audio/AudioDevice.h"

#include "Rendering/Core/API/ITexture.h"
#include "Mesh.h"
#include "Material.h"
#include "Audio/SoundEffect3D.h"
#include "Game/Scene.h"

namespace LambdaEngine
{
	class LAMBDA_API ResourceLoader
	{
	public:
		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pGraphicsDevice - A Graphics Device
		*	pDir - Path to the directory that holds the .obj file
		*	pFilename - The name of the .obj file
		*	loadedGameObjects - A vector where all loaded GameObject(s) will be stored, th GUIDs of each GameObject is an index to the loadedMeshes and loadedMaterials vectors
		*	loadedMeshes - A vector where all loaded Mesh(es) will be stored
		*	loadedMaterials - A vector where all loaded Material(s) will be stored
		*	loadedTextures - A vector where all loaded Texture(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(IGraphicsDevice* pGraphicsDevice, const char* pDir, const char* pFilename, std::vector<GameObject>& loadedGameObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures);

		/*
		* Load a mesh from file
		*	pGraphicsDevice - A Graphics Device
		*	pFilepath - Path to the .obj file
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath);

		/*
		* Load a mesh from memory
		*	pGraphicsDevice - A Graphics Device
		*	pVertices - An array of vertices
		*	numVertices - The vertexcount
		*	pIndices - An array of indices
		*	numIndices - The Indexcount
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromMemory(IGraphicsDevice* pGraphicsDevice, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a texture from file
		*	pGraphicsDevice - A Graphics Device
		*	pFilepath - Path to the texture file
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureFromFile(IGraphicsDevice* pGraphicsDevice, const char* pFilepath);

		/*
		* Load a texture from memory
		*	pGraphicsDevice - A Graphics Device
		*	pData - The pixeldata
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureFromMemory(IGraphicsDevice* pGraphicsDevice, const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	pGraphicsDevice - An audio device
		*	pFilepath - Path to the audio file
		* return - an SoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static SoundEffect3D* LoadSoundFromFile(AudioDevice* pAudioDevice, const char* pFilepath);

	private:
		static bool ReadDataFromFile(const char* pFilepath, byte** ppData, uint32* pDataSize);
	};
}