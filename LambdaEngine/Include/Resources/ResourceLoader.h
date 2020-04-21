#pragma once
#include "Audio/API/IAudioDevice.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/IShader.h"

#include "Audio/API/ISoundEffect3D.h"

#include "Game/Scene.h"

#include "Material.h"
#include "Mesh.h"

namespace LambdaEngine
{
	class LAMBDA_API ResourceLoader
	{
	public:

		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pDir - Path to the directory that holds the .obj file
		*	pFilename - The name of the .obj file
		*	loadedGameObjects - A vector where all loaded GameObject(s) will be stored, th GUIDs of each GameObject is an index to the loadedMeshes and loadedMaterials vectors
		*	loadedMeshes - A vector where all loaded Mesh(es) will be stored
		*	loadedMaterials - A vector where all loaded Material(s) will be stored
		*	loadedTextures - A vector where all loaded Texture(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GameObject>& loadedGameObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures);

		/*
		* Load a mesh from file
		*	pFilepath - Path to the .obj file
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromFile(const char* pFilepath);

		/*
		* Load a mesh from memory
		*	pVertices - An array of vertices
		*	numVertices - The vertexcount
		*	pIndices - An array of indices
		*	numIndices - The Indexcount
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a texture from file
		*	pFilepath - Path to the texture file
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureFromFile(const char* pFilepath, EFormat format, bool generateMips);

		/*
		* Load a texture from memory
		*	pName - Name of the texture
		*	pData - The pixeldata
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureFromMemory(const char* pName, const void* pData, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	pFilepath - Path to the shader file
		*	stage - Which stage the shader belongs to
		*	lang - The language of the shader file
		*	pConstants - Optional shader constants, can be nullptr
		*	shaderConstantCount - The number of entries in pConstants
		*	pEntryPoint - The name of the shader entrypoint
		* return - an IShader* if the shader was loaded, otherwise nullptr will be returned
		*/
		static IShader* LoadShaderFromFile(const char* pFilepath, FShaderStageFlags stage, EShaderLang lang, ShaderConstant* pConstants = nullptr, uint32 shaderConstantCount = 0, const char* pEntryPoint = "main");

		/*
		* Load sound from file
		*	pFilepath - Path to the audio file
		* return - an ISoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static ISoundEffect3D* LoadSoundEffectFromFile(const char* pFilepath);

	private:
		static bool ReadDataFromFile(const char* pFilepath, byte** ppData, uint32* pDataSize);
        static void ConvertBackslashes(std::string& string);

	private:
		static ICommandAllocator*		s_pCopyCommandAllocator;
		static ICommandList*			s_pCopyCommandList;
		static IFence*					s_pCopyFence;
		static uint64					s_SignalValue;
	};
}
