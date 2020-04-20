#pragma once
#include "ResourceLoader.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class IGraphicsDevice;
	class IAudioDevice;

	//Meshes

	//Meshes
	constexpr GUID_Lambda DEFAULT_MATERIAL = 0;

	//Textures
	constexpr GUID_Lambda DEFAULT_COLOR_MAP = DEFAULT_MATERIAL + 1;
	constexpr GUID_Lambda DEFAULT_NORMAL_MAP = DEFAULT_COLOR_MAP + 1;

	constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID = DEFAULT_NORMAL_MAP + 1;

	constexpr GUID_Lambda GUID_NONE = UINT32_MAX;

	class LAMBDA_API ResourceManager
	{
	public:
		DECL_REMOVE_COPY(ResourceManager);
		DECL_REMOVE_MOVE(ResourceManager);

		ResourceManager(IGraphicsDevice* pGraphicsDevice, IAudioDevice* pAudioDevice);
		~ResourceManager();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pGraphicsDevice - A Graphics Device
		*	pDir - Path to the directory that holds the .obj file
		*	pFilename - The name of the .obj file
		*	result - A vector where all loaded GameObject(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		bool LoadSceneFromFile(const char* pDir, const char* pFilename, std::vector<GameObject>& result);

		/*
		* Load a mesh from file
		*	pFilepath - Path to the .obj file
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadMeshFromFile(const char* pFilepath);

		/*
		* Load a mesh from memory
		*	pVertices - An array of vertices
		*	numVertices - The vertexcount
		*	pIndices - An array of indices
		*	numIndices - The Indexcount
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadMeshFromMemory(const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a material from memory
		*	albedoMap, normalMap, ambientOcclusionMap, metallicMap, roughnessMap - The GUID of a valid ITexture loaded with this ResourceManager, or GUID_NONE to use default maps
		*	properties - Material Properties which are to be used for this material
		* return - a valid GUID if the materials was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadMaterialFromMemory(GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		/*
		* Load a texture from file
		*	pFilepath - Path to the texture file
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadTextureFromFile(const char* pFilepath, EFormat format, bool generateMips);

		/*
		* Load a texture from memory
		*	pName - Name of the texture
		*	pData - The pixeldata
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadTextureFromMemory(const char* pName, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	pFilepath - Path to the shader file
		*	stage - Which stage the shader belongs to
		*	lang - The language of the shader file
		*	pConstants - Optional shader constants, can be nullptr
		*	shaderConstantCount - The number of entries in pConstants
		*	pEntryPoint - The name of the shader entrypoint
		* return - a valid GUID if the shader was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadShaderFromFile(const char* pFilepath, FShaderStageFlags stage, EShaderLang lang, ShaderConstant* pConstants = nullptr, uint32 shaderConstantCount = 0, const char* pEntryPoint = "main");

		/*
		* Load sound from file
		*	pFilepath - Path to the audio file
		* return - a valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		GUID_Lambda LoadSoundEffectFromFile(const char* pFilepath);

		Mesh*					GetMesh(GUID_Lambda guid);
		const Mesh*				GetMesh(GUID_Lambda guid) const;

		Material*				GetMaterial(GUID_Lambda guid);
		const Material*			GetMaterial(GUID_Lambda guid) const;

		ITexture*				GetTexture(GUID_Lambda guid);
		const ITexture*			GetTexture(GUID_Lambda guid) const;
		
		ITextureView*			GetTextureView(GUID_Lambda guid);
		const ITextureView*		GetTextureView(GUID_Lambda guid) const;

		IShader*				GetShader(GUID_Lambda guid);
		const IShader*			GetShader(GUID_Lambda guid) const;

		ISoundEffect3D*			GetSoundEffect(GUID_Lambda guid);
		const ISoundEffect3D*	GetSoundEffect(GUID_Lambda guid) const;

	private:
		GUID_Lambda RegisterLoadedMesh(Mesh* pMesh);
		GUID_Lambda RegisterLoadedMaterial(Material* pMaterial);
		GUID_Lambda RegisterLoadedTexture(ITexture* pTexture);

		void InitDefaultResources();

	private:
		IGraphicsDevice* m_pGraphicsDevice;
		IAudioDevice* m_pAudioDevice;

		std::unordered_map<GUID_Lambda, Mesh*>	   m_Meshes;
		std::unordered_map<GUID_Lambda, Material*> m_Materials;
		std::unordered_map<GUID_Lambda, ITexture*> m_Textures;
		std::unordered_map<GUID_Lambda, ITextureView*> m_TextureViews;
		std::unordered_map<GUID_Lambda, IShader*> m_Shaders;
		std::unordered_map<GUID_Lambda, ISoundEffect3D*> m_SoundEffects;

	private:
		static GUID_Lambda s_NextFreeGUID;

	};
}