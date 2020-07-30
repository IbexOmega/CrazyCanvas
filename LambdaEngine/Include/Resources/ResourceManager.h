#pragma once
#include "ResourceLoader.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	union ShaderConstant;

	class IGraphicsDevice;
	class IAudioDevice;

	//Meshes

	//Meshes
	constexpr GUID_Lambda DEFAULT_MATERIAL = 0;

	//Textures
	constexpr GUID_Lambda DEFAULT_COLOR_MAP = DEFAULT_MATERIAL + 1;
	constexpr GUID_Lambda DEFAULT_NORMAL_MAP = DEFAULT_COLOR_MAP + 1;

	constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID = DEFAULT_NORMAL_MAP + 1;

	constexpr const char* SCENE_DIR			= "../Assets/Scenes/";
	constexpr const char* MESH_DIR			= "../Assets/Meshes/";
	constexpr const char* TEXTURE_DIR		= "../Assets/Textures/";
	constexpr const char* SHADER_DIR		= "../Assets/Shaders/";
	constexpr const char* SOUND_DIR			= "../Assets/Sounds/";

	class LAMBDA_API ResourceManager
	{
		struct ShaderLoadDesc
		{
			String				Filepath				= "";
			FShaderStageFlags	Stage					= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			EShaderLang			Lang					= EShaderLang::NONE;
			const char*			pEntryPoint				= nullptr;
		};

	public:
		DECL_STATIC_CLASS(ResourceManager);

		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pGraphicsDevice - A Graphics Device
		*	pDir - Path to the directory that holds the .obj file
		*	filename - The name of the .obj file
		*	result - A vector where all loaded GameObject(s) will be stored
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(const String& filename, std::vector<GameObject>& result);

		/*
		* Load a mesh from file
		*	filename - The name of the .obj file
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromFile(const String& filename);

		/*
		* Load a mesh from memory
		*	name - A name given to the mesh resource
		*	pVertices - An array of vertices
		*	numVertices - The vertexcount
		*	pIndices - An array of indices
		*	numIndices - The Indexcount
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromMemory(const String& name, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a material from memory
		*	name - A name given to the material
		*	albedoMap, normalMap, ambientOcclusionMap, metallicMap, roughnessMap - The GUID of a valid ITexture loaded with this ResourceManager, or GUID_NONE to use default maps
		*	properties - Material Properties which are to be used for this material
		* return - a valid GUID if the materials was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMaterialFromMemory(const String& name, GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		/*
		* Load a texture from file
		*	filename - Name of the texture file
		*	format - The format of the pixeldata
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromFile(const String& filename, EFormat format, bool generateMips);

		/*
		* Load a texture from memory
		*	name - A Name of given to the texture
		*	pData - The pixeldata
		*	width - The pixel width of the texture
		*	height - The pixel height of the texture
		*	format - The format of the pixeldata
		*	usageFlags - Usage flags
		*	generateMips - If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromMemory(const String& name, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips);

		/*
		* Load sound from file
		*	filename - Name of the shader file
		*	stage - Which stage the shader belongs to
		*	lang - The language of the shader file
		*	pEntryPoint - The name of the shader entrypoint
		* return - a valid GUID if the shader was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadShaderFromFile(const String& filename, FShaderStageFlags stage, EShaderLang lang, const char* pEntryPoint = "main");

		/*
		* Load sound from file
		*	filename - Name of the audio file
		* return - a valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadSoundEffectFromFile(const String& filename);

		static void ReloadAllShaders();

		static GUID_Lambda GetMeshGUID(const String& name);
		static GUID_Lambda GetMaterialGUID(const String& name);
		static GUID_Lambda GetTextureGUID(const String& name);
		static GUID_Lambda GetShaderGUID(const String& name);
		static GUID_Lambda GetSoundEffectGUID(const String& name);

		static Mesh*					GetMesh(GUID_Lambda guid);
		static Material*				GetMaterial(GUID_Lambda guid);
		static ITexture*				GetTexture(GUID_Lambda guid);
		static ITextureView*			GetTextureView(GUID_Lambda guid);
		static IShader*					GetShader(GUID_Lambda guid);
		static ISoundEffect3D*			GetSoundEffect(GUID_Lambda guid);

	private:
		static GUID_Lambda RegisterLoadedMesh(Mesh* pMesh);
		static GUID_Lambda RegisterLoadedMaterial(Material* pMaterial);
		static GUID_Lambda RegisterLoadedTexture(ITexture* pTexture);

		static GUID_Lambda GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name);

		static void InitDefaultResources();

	private:
		static GUID_Lambda											s_NextFreeGUID;

		static std::unordered_map<String, GUID_Lambda>				s_MeshNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>				s_MaterialNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>				s_TextureNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>				s_ShaderNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>				s_SoundEffectNamesToGUIDs;

		static std::unordered_map<GUID_Lambda, Mesh*>				s_Meshes;
		static std::unordered_map<GUID_Lambda, Material*>			s_Materials;
		static std::unordered_map<GUID_Lambda, ITexture*>			s_Textures;
		static std::unordered_map<GUID_Lambda, ITextureView*>		s_TextureViews;
		static std::unordered_map<GUID_Lambda, IShader*>			s_Shaders;
		static std::unordered_map<GUID_Lambda, ISoundEffect3D*>		s_SoundEffects;

		static std::unordered_map<GUID_Lambda, ShaderLoadDesc>		s_ShaderLoadConfigurations;
	};
}
