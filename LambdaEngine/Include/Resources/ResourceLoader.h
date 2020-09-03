#pragma once
#include "Audio/API/IAudioDevice.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/IShader.h"
#include "Rendering/Core/API/IDeviceAllocator.h"

#include "Audio/API/ISoundEffect3D.h"

#include "Game/Scene.h"

#include "Material.h"
#include "Mesh.h"

namespace glslang
{
	class TIntermediate;
}

namespace LambdaEngine
{
	class LAMBDA_API ResourceLoader
	{
	public:

		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	filepath - Path to the .obj file
		*	loadedGameObjects - A vector where all loaded GameObject(s) will be stored, th GUIDs of each GameObject is an index to the loadedMeshes and loadedMaterials vectors
		*	loadedMeshes - A vector where all loaded Mesh(es) will be stored
		*	loadedMaterials - A vector where all loaded Material(s) will be stored
		*	loadedTextures - A vector where all loaded Texture(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(const String& filepath, std::vector<GameObject>& loadedGameObjects, std::vector<Mesh*>& loadedMeshes, std::vector<Material*>& loadedMaterials, std::vector<ITexture*>& loadedTextures);

		/*
		* Load a mesh from file
		*	filepath - Path to the .obj file
		* return - a Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static Mesh* LoadMeshFromFile(const String& filepath);

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
		*	name - Name of the texture
		*	dir - A root directory to be appended to entries of pFilepaths
		*	pFilenames - Names of the texture files
		*	count - Number of elements in pFilenames
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureArrayFromFile(const String& name, const String& dir, const String* pFilenames, uint32 count, EFormat format, bool generateMips);

		/*
		* Load a texture from memory
		*	name - Name of the texture
		*	ppData - An array of pixeldata
		*	arrayCount - Number of elements in ppData
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - an ITexture* if the texture was loaded, otherwise nullptr will be returned
		*/
		static ITexture* LoadTextureArrayFromMemory(const String& name, const void* const * ppData, uint32 arrayCount, uint32 width, uint32 height, EFormat format, uint32 usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	filepath - Path to the shader file
		*	stage - Which stage the shader belongs to
		*	lang - The language of the shader file
		*	pEntryPoint - The name of the shader entrypoint
		* return - an IShader* if the shader was loaded, otherwise nullptr will be returned
		*/
		static IShader* LoadShaderFromFile(const String& filepath, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint = "main");

		static bool CreateShaderReflection(const String& filepath, FShaderStageFlags stage, EShaderLang lang, ShaderReflection* pReflection);

		/*
		* Load sound from file
		*	filepath - Path to the audio file
		* return - an ISoundEffect3D* if the sound was loaded, otherwise nullptr will be returned
		*/
		static ISoundEffect3D* LoadSoundEffectFromFile(const String& filepath);

	private:
		static bool ReadDataFromFile(const String& filepath, const char* pMode, byte** ppData, uint32* pDataSize);
        static void ConvertBackslashes(std::string& string);

		static bool CompileGLSLToSPIRV(const String& filepath, const char* pSource, int32 sourceSize, FShaderStageFlags stage, std::vector<uint32>* pSourceSPIRV, ShaderReflection* pReflection);
		static bool CreateShaderReflection(glslang::TIntermediate* pIntermediate, FShaderStageFlags stage, ShaderReflection* pReflection);

	private:
		static IDeviceAllocator*		s_pAllocator;
		static ICommandAllocator*		s_pCopyCommandAllocator;
		static ICommandList*			s_pCopyCommandList;
		static IFence*					s_pCopyFence;
		static uint64					s_SignalValue;
	};
}
