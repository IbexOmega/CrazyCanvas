#pragma once
#include "ResourceLoader.h"

#include "Containers/TSet.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"

#include "Application/API/Events/DebugEvents.h"

namespace LambdaEngine
{
	union ShaderConstant;

	class GraphicsDevice;
	class IAudioDevice;

	//Meshes

	//Meshes
	constexpr GUID_Lambda GUID_MESH_QUAD					= 0;

	//Material
	constexpr GUID_Lambda GUID_MATERIAL_DEFAULT				= GUID_MESH_QUAD + 1;

	//Textures
	constexpr GUID_Lambda GUID_TEXTURE_DEFAULT_COLOR_MAP	= GUID_MATERIAL_DEFAULT + 1;
	constexpr GUID_Lambda GUID_TEXTURE_DEFAULT_NORMAL_MAP	= GUID_TEXTURE_DEFAULT_COLOR_MAP + 1;
	constexpr GUID_Lambda GUID_TEXTURE_DEFAULT_MASK_MAP		= GUID_TEXTURE_DEFAULT_NORMAL_MAP + 1;

	constexpr GUID_Lambda SMALLEST_UNRESERVED_GUID			= GUID_TEXTURE_DEFAULT_MASK_MAP + 1;

	constexpr const char* SCENE_DIR			= "../Assets/Scenes/";
	constexpr const char* MESH_DIR			= "../Assets/Meshes/";
	constexpr const char* ANIMATIONS_DIR	= MESH_DIR; // Equal to mesh dir for now
	constexpr const char* TEXTURE_DIR		= "../Assets/Textures/";
	constexpr const char* SHADER_DIR		= "../Assets/Shaders/";
	constexpr const char* SOUND_DIR			= "../Assets/Sounds/";

	struct SceneLoadDesc
	{
		String	Filename = "";
		TArray<LevelObjectOnLoadDesc> LevelObjectDescriptions = {};
	};

	class LAMBDA_API ResourceManager
	{
		struct MaterialLoadDesc
		{
			GUID_Lambda AlbedoMapGUID				= GUID_NONE;
			GUID_Lambda NormalMapGUID				= GUID_NONE;
			GUID_Lambda AOMapGUID					= GUID_NONE;
			GUID_Lambda MetallicMapGUID				= GUID_NONE;
			GUID_Lambda RoughnessMapGUID			= GUID_NONE;
			GUID_Lambda AOMetallicRoughnessMapGUID	= GUID_NONE;
		};

		struct ShaderLoadDesc
		{
			String				Filepath	= "";
			FShaderStageFlag	Stage		= FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
			EShaderLang			Lang		= EShaderLang::SHADER_LANG_NONE;
			const char*			pEntryPoint	= nullptr;
		};

	public:
		DECL_STATIC_CLASS(ResourceManager);

		static bool Init();
		static bool Release();

		/*
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		*	pSceneLoadDesc		- A load desc, containing the filename and the definition of special objects
		*	meshComponents		- A vector where all loaded MeshComponent(s) will be stored
		*	directionalLights	- A vector where all loaded LoadedDirectionalLight(s) will be stored
		*	pointLights			- A vector where all loaded LoadedPointLight(s) will be stored
		*	levelObjects		- A vector where all loaded LevelObject(s) will be stored according to the definition given in SceneLoadDesc::LevelObjectDescriptions
		* return - true if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(
			const SceneLoadDesc* pSceneLoadDesc,
			TArray<MeshComponent>& meshComponents,
			TArray<LoadedDirectionalLight>& directionalLights,
			TArray<LoadedPointLight>& pointLights,
			TArray<LevelObjectOnLoad>& levelObjects,
			const String& directory = SCENE_DIR);

		/*
		* Load a mesh from file
		*	filename - The name of the file
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromFile(const String& filename);

		/*
		* Load a mesh from file
		*	filename	- The name of the file
		*	animations	- TArray with valid GUIDs for all the animations
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromFile(const String& filename, TArray<GUID_Lambda>& animations);

		/*
		* Load a mesh from file
		*	filename	- The name of the file
		* return - a TArray with valid GUIDs if the animations was loaded, otherwise returns an empty TArray
		*/
		static TArray<GUID_Lambda> LoadAnimationsFromFile(const String& filename);

		/*
		* Load a mesh from memory
		*	name		- A name given to the mesh resource
		*	pVertices	- An array of vertices
		*	numVertices	- The vertexcount
		*	pIndices	- An array of indices
		*	numIndices	- The Indexcount
		* return - a valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMeshFromMemory(const String& name, const Vertex* pVertices, uint32 numVertices, const uint32* pIndices, uint32 numIndices);

		/*
		* Load a material from memory
		*	name				- A name given to the material
		*	albedoMap, 
		*	normalMap, 
		*	ambientOcclusionMap, 
		*	metallicMap, 
		*	roughnessMap		- The GUID of a valid Texture loaded with this ResourceManager, or GUID_NONE to use default maps
		*	properties			- Material Properties which are to be used for this material
		* return - a valid GUID if the materials was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMaterialFromMemory(const String& name, GUID_Lambda albedoMap, GUID_Lambda normalMap, GUID_Lambda ambientOcclusionMap, GUID_Lambda metallicMap, GUID_Lambda roughnessMap, const MaterialProperties& properties);

		/*
		* Load multiple textures from file and combine in a Texture Array
		*	name			- A Name of given to the textureArray
		*	pFilenames		- Names of the texture files
		*	count			- number of elements in pFilenames
		*	format			- The format of the pixeldata
		*	generateMips	- If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips, bool linearFilteringMips);

		/*
		* Load multiple Cube textures from file and combine into Texture Arrays along with TextureViews and CubeTextureViews
		*	name			- A Name of given to the cubeTexture
		*	pFilenames		- Names of the texture files
		*	count			- number of cubeTextures to load
		*	format			- The format of the pixeldata
		*	generateMips	- If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadCubeTexturesArrayFromFile(const String& name, const String* pFilenames, uint32 count, EFormat format, bool generateMips, bool linearFilteringMips);

		/*
		* Load a texture from file
		*	filename		- Name of the texture file
		*	format			- The format of the pixeldata
		*	generateMips	- If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromFile(const String& filename, EFormat format, bool generateMips, bool linearFilteringMips);

		/*
		* Load a texture from memory
		*	name			- A Name of given to the texture
		*	pData			- The pixeldata
		*	width			- The pixel width of the texture
		*	height			- The pixel height of the texture
		*	format			- The format of the pixeldata
		*	usageFlags		- Usage flags
		*	generateMips	- If mipmaps should be generated on load
		* return - a valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromMemory(const String& name, const void* pData, uint32_t width, uint32_t height, EFormat format, uint32_t usageFlags, bool generateMips, bool linearFilteringMips);

		/*
		* Load sound from file
		*	filename	- Name of the shader file
		*	stage		- Which stage the shader belongs to
		*	lang		- The language of the shader file
		*	pEntryPoint	- The name of the shader entrypoint
		* return - a valid GUID if the shader was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadShaderFromFile(const String& filename, FShaderStageFlag stage, EShaderLang lang, const char* pEntryPoint = "main");

		static GUID_Lambda RegisterShader(const String& name, Shader* pShader);

		/*
		* Load sound from file
		*	filename - Name of the audio file
		* return - a valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadSoundEffectFromFile(const String& filename);

		/*
		* Combine PBR materials into one texture
		*/
		static GUID_Lambda CombineMaterialTextures(
			Material* pMaterial,
			Texture* pAOMap,
			Texture* pMetallicMap,
			Texture* pRoughnessMap,
			TextureView* pAOMapView,
			TextureView* pMetallicMapView,
			TextureView* pRoughnessMapView);

		static bool UnloadMesh(GUID_Lambda guid);
		static bool UnloadMaterial(GUID_Lambda guid);
		static bool UnloadAnimation(GUID_Lambda guid);
		static bool UnloadTexture(GUID_Lambda guid);
		static bool UnloadShader(GUID_Lambda guid);
		static bool UnloadSoundEffect(GUID_Lambda guid);

		static bool DecrementTextureMaterialRef(GUID_Lambda guid);

		static GUID_Lambda GetMeshGUID(const String& name);
		static GUID_Lambda GetMaterialGUID(const String& name);
		static GUID_Lambda GetAnimationGUID(const String& name);
		static bool GetAnimationGUIDsFromMeshName(const String& name, TArray<GUID_Lambda>& guids);
		static GUID_Lambda GetTextureGUID(const String& name);
		static GUID_Lambda GetShaderGUID(const String& name);
		static GUID_Lambda GetSoundEffectGUID(const String& name);

		static Mesh*			GetMesh(GUID_Lambda guid);
		static Material*		GetMaterial(GUID_Lambda guid);
		static Animation*		GetAnimation(GUID_Lambda guid);
		static Texture*			GetTexture(GUID_Lambda guid);
		static TextureView*		GetTextureView(GUID_Lambda guid);
		static Shader*			GetShader(GUID_Lambda guid);
		static ISoundEffect3D*	GetSoundEffect(GUID_Lambda guid);

		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetMeshNamesMap()			{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetMaterialNamesMap()		{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetAnimationNamesMap()		{ return s_AnimationNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetTextureNamesMap()		{ return s_TextureNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetShaderNamesMap()			{ return s_ShaderNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetSoundEffectNamesMap()	{ return s_SoundEffectNamesToGUIDs; }

		FORCEINLINE static std::unordered_map<GUID_Lambda, Mesh*>&				GetMeshGUIDMap()		{ return s_Meshes; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Material*>&			GetMaterialGUIDMap()	{ return s_Materials; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Texture*>&			GetTextureGUIDMap()		{ return s_Textures; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, TextureView*>&		GetTextureViewGUIDMap()	{ return s_TextureViews; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Shader*>&			GetShaderGUIDMap()		{ return s_Shaders; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, ISoundEffect3D*>&	GetSoundEffectGUIDMap()	{ return s_SoundEffects; }

	private:
		static bool OnShaderRecompileEvent(const ShaderRecompileEvent& event);

		static GUID_Lambda RegisterLoadedMesh(const String& name, Mesh* pMesh);
		static GUID_Lambda RegisterLoadedMaterial(const String& name, Material* pMaterial);
		static GUID_Lambda RegisterLoadedAnimation(const String& name, Animation* pAnimation);
		static GUID_Lambda RegisterLoadedTexture(Texture* pTexture);
		static GUID_Lambda RegisterLoadedTextureWithView(Texture* pTexture, TextureView* pTextureView);

		static GUID_Lambda GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name);

		static void InitMaterialCreation();
		static void InitDefaultResources();

		static void ReleaseMaterialCreation();

	private:
		static GUID_Lambda s_NextFreeGUID;

		static std::unordered_map<GUID_Lambda, String>			s_MeshGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String>			s_MaterialGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String>			s_AnimationGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String>			s_TextureGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String>			s_ShaderGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String>			s_SoundEffectGUIDsToNames;

		static std::unordered_map<String, GUID_Lambda>			s_MeshNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_MaterialNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_AnimationNamesToGUIDs;
		static std::unordered_map<String, TArray<GUID_Lambda>>	s_FileNamesToAnimationGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_TextureNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_ShaderNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_SoundEffectNamesToGUIDs;

		static std::unordered_map<GUID_Lambda, Mesh*>				s_Meshes;
		static std::unordered_map<GUID_Lambda, Material*>			s_Materials;
		static std::unordered_map<GUID_Lambda, Animation*>			s_Animations;
		static std::unordered_map<GUID_Lambda, Texture*>			s_Textures;
		static std::unordered_map<GUID_Lambda, TextureView*>		s_TextureViews;
		static std::unordered_map<GUID_Lambda, Shader*>				s_Shaders;
		static std::unordered_map<GUID_Lambda, ISoundEffect3D*>		s_SoundEffects;

		static std::unordered_map<GUID_Lambda, uint32>				s_TextureMaterialRefs;
		static std::unordered_map<GUID_Lambda, MaterialLoadDesc>	s_MaterialLoadConfigurations;
		static std::unordered_map<GUID_Lambda, ShaderLoadDesc>		s_ShaderLoadConfigurations;

		//Material Combine 
		static CommandAllocator* s_pMaterialComputeCommandAllocator;
		static CommandAllocator* s_pMaterialGraphicsCommandAllocator;

		static CommandList* s_pMaterialComputeCommandList;
		static CommandList* s_pMaterialGraphicsCommandList;

		static Fence* s_pMaterialFence;

		static DescriptorHeap* s_pMaterialDescriptorHeap;
		static DescriptorSet* s_pMaterialDescriptorSet;

		static PipelineLayout* s_pMaterialPipelineLayout;
		static PipelineState* s_pMaterialPipelineState;

		static GUID_Lambda s_MaterialShaderGUID;

		static TSet<GUID_Lambda> s_UnloadedGUIDs;
	};
}
