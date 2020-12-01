#pragma once
#include "ResourceLoader.h"
#include "ResourcePaths.h"

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

	struct SceneLoadDesc
	{
		String	Filename = "";
		TArray<LevelObjectOnLoadDesc> LevelObjectDescriptions = {};
	};

	class LAMBDA_API ResourceManager
	{
		struct ShaderLoadDesc
		{
			String				Filepath	= "";
			FShaderStageFlag	Stage		= FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
			EShaderLang			Lang		= EShaderLang::SHADER_LANG_NONE;
			const char*			pEntryPoint	= nullptr;
		};

	public:
		struct MaterialLoadDesc
		{
			GUID_Lambda AlbedoMapGUID = GUID_NONE;
			GUID_Lambda NormalMapGUID = GUID_NONE;
			GUID_Lambda AOMapGUID = GUID_NONE;
			GUID_Lambda MetallicMapGUID = GUID_NONE;
			GUID_Lambda RoughnessMapGUID = GUID_NONE;
			GUID_Lambda AOMetallicRoughnessMapGUID = GUID_NONE;
		};

		DECL_STATIC_CLASS(ResourceManager);

		static bool Init();
		static bool Release();

		/**
		* Load a Scene from file, (experimental, only tested with Sponza Scene)
		* @param pSceneLoadDesc		A load desc, containing the filename and the definition of special objects
		* @param meshComponents		A vector where all loaded MeshComponent(s) will be stored
		* @param directionalLights	A vector where all loaded LoadedDirectionalLight(s) will be stored
		* @param pointLights		A vector where all loaded LoadedPointLight(s) will be stored
		* @param levelObjects		A vector where all loaded LevelObject(s) will be stored according to the definition given in SceneLoadDesc::LevelObjectDescriptions
		* @return True if the scene was loaded, false otherwise
		*/
		static bool LoadSceneFromFile(
			const SceneLoadDesc* pSceneLoadDesc,
			TArray<MeshComponent>& meshComponents,
			TArray<LoadedDirectionalLight>& directionalLights,
			TArray<LoadedPointLight>& pointLights,
			TArray<LevelObjectOnLoad>& levelObjects,
			const String& directory = SCENE_DIR);

		/**
		* Load a mesh from file
		* @param filename	The name of the file
		* @param meshGUID	The loaded Mesh GUID
		* @return A valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static void LoadMeshFromFile(const String& filename, GUID_Lambda& meshGUID, bool shouldTessellate);

		/**
		* Load a mesh from file
		* @param filename	The name of the file
		* @param meshGUID	The loaded Mesh GUID
		* @param animations	TArray with valid GUIDs for all the animations
		* @return A valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static void LoadMeshFromFile(const String& filename, GUID_Lambda& meshGUID, TArray<GUID_Lambda>& animations, bool shouldTessellate);

		/**
		* Load a mesh from file
		* @param filename		The name of the file
		* @param meshGUID		The loaded Mesh GUID
		* @param materialGUID	The loaded Material GUID
		* @return A valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static void LoadMeshAndMaterialFromFile(const String& filename, GUID_Lambda& meshGUID, GUID_Lambda& materialGUID, bool shouldTessellate);

		/**
		* Load a mesh from file
		* @param filename		The name of the file
		* @param meshGUID		The loaded Mesh GUID
		* @param materialGUID	The loaded Material GUID
		* @param animations		TArray with valid GUIDs for all the animations
		* @return A valid GUID if the mesh was loaded, otherwise returns GUID_NONE
		*/
		static void LoadMeshAndMaterialFromFile(
			const String& filename, 
			GUID_Lambda& meshGUID, 
			GUID_Lambda& materialGUID, 
			TArray<GUID_Lambda>& animations, 
			bool shouldTessellate);

		/**
		* Load a mesh from file
		* @param filename The name of the file
		* @return A TArray with valid GUIDs if the animations was loaded, otherwise returns an empty TArray
		*/
		static TArray<GUID_Lambda> LoadAnimationsFromFile(const String& filename);

		/**
		* Load a mesh from memory
		* @param name				Name of the texture
		* @param pVertices			An array of vertices
		* @param numVertices		The vertexcount
		* @param pIndices			An array of indices
		* @param numIndices			The Indexcount
		* @param useMeshletCache	Enables read/write to files containing generated meshlets
		* @return A Mesh* if the mesh was loaded, otherwise nullptr will be returned
		*/
		static GUID_Lambda LoadMeshFromMemory(
			const String& name,
			const Vertex* pVertices,
			uint32 numVertices,
			const uint32* pIndices,
			uint32 numIndices,
			bool useMeshletCache = false);

		/**
		* Load a material from memory
		* @param name				A name given to the material
		* @param albedoMap
		* @param normalMap
		* @param ambientOcclusionMap
		* @param metallicMap
		* @param roughnessMap		The GUID of a valid Texture loaded with this ResourceManager, or GUID_NONE to use default maps
		* @param properties			Material Properties which are to be used for this material
		* @return A valid GUID if the materials was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMaterialFromMemory(
			const String& name,
			GUID_Lambda albedoMap,
			GUID_Lambda normalMap,
			GUID_Lambda ambientOcclusionMap,
			GUID_Lambda metallicMap,
			GUID_Lambda roughnessMap,
			const MaterialProperties& properties);

		/**
		* Load multiple textures from file and combine in a Texture Array
		* @param name			- A Name of given to the textureArray
		* @param pFilenames		- Names of the texture files
		* @param count			- number of elements in pFilenames
		* @param format			- The format of the pixeldata
		* @param generateMips	- If mipmaps should be generated on load
		* @return A valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureArrayFromFile(
			const String& name,
			const String* pFilenames,
			uint32 count,
			EFormat format,
			bool generateMips,
			bool linearFilteringMips);

		/**
		* Load multiple Cube textures from file and combine into Texture Arrays along with TextureViews and CubeTextureViews
		* @param name			A Name of given to the cubeTexture
		* @param pFilenames		Names of the texture files
		* @param count			number of cubeTextures to load
		* @param format			The format of the pixeldata
		* @param generateMips	If mipmaps should be generated on load
		* @return A valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadCubeTexturesArrayFromFile(
			const String& name,
			const String* pFilenames,
			uint32 count,
			EFormat format,
			bool generateMips,
			bool linearFilteringMips);

		/**
		* Load a texture from file
		* @param filename		Name of the texture file
		* @param format			The format of the pixeldata
		* @param generateMips	If mipmaps should be generated on load
		* @return A valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromFile(
			const String& filename,
			EFormat format,
			bool generateMips,
			bool linearFilteringMips);

		/**
		* Load a texture from file
		* @param filename		Name of the texture file, this should be a equirectangular map. This can be both HDR and LDR format
		* @param format			The format of the pixeldata, Should be R8G8B8A8 or R32G32B32A32
		* @param generateMips	If mipmaps should be generated on load
		* @return A valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureCubeFromPanormaFile(
			const String& filename,
			EFormat format,
			uint32 size,
			bool generateMips);

		/**
		* Load a texture from memory
		* @param name			A Name of given to the texture
		* @param pData			The pixeldata
		* @param width			The pixel width of the texture
		* @param height			The pixel height of the texture
		* @param format			The format of the pixeldata
		* @param usageFlags		Usage flags
		* @param generateMips	If mipmaps should be generated on load
		* @return A valid GUID if the texture was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadTextureFromMemory(
			const String& name,
			const void* pData,
			uint32_t width,
			uint32_t height,
			EFormat format,
			uint32_t usageFlags,
			bool generateMips,
			bool linearFilteringMips);

		/**
		* Load sound from file
		* @param filename		Name of the shader file
		* @param stage			Which stage the shader belongs to
		* @param lang			The language of the shader file
		* @param pEntryPoint	The name of the shader entrypoint
		* @return A valid GUID if the shader was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadShaderFromFile(const String& filename, FShaderStageFlag stage, EShaderLang lang, const char* pEntryPoint = "main");

		static GUID_Lambda RegisterShader(const String& name, Shader* pShader);

		/**
		* Load sound 3D from file
		* @param filename Name of the audio file
		* @return A valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadSoundEffect3DFromFile(const String& filename);

		/**
		* Load sound 2D from file
		* @param filename Name of the audio file
		* @return A valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadSoundEffect2DFromFile(const String& filename);

		/**
		* Load music from file
		* @param filename Name of the audio file
		* @return A valid GUID if the sound was loaded, otherwise returns GUID_NONE
		*/
		static GUID_Lambda LoadMusicFromFile(const String& filename, float32 defaultVolume = 1.0f, float32 defaultPitch = 1.0f);

		/*
		* Combine PBR materials into one texture
		*/
		static GUID_Lambda CombineMaterialTextures(
			const String& combinedTextureName,
			Material* pMaterial,
			Texture* pAOMap,
			Texture* pMetallicMap,
			Texture* pRoughnessMap,
			Texture* pMetallicRoughnessMap,
			TextureView* pAOMapView,
			TextureView* pMetallicMapView,
			TextureView* pRoughnessMapView,
			TextureView* pMetallicRoughnessMapView);

		static bool UnloadMesh(GUID_Lambda guid);
		static bool UnloadMaterial(GUID_Lambda guid);
		static bool UnloadAnimation(GUID_Lambda guid);
		static bool UnloadTexture(GUID_Lambda guid);
		static bool UnloadShader(GUID_Lambda guid);
		static bool UnloadSoundEffect3D(GUID_Lambda guid);
		static bool UnloadSoundEffect2D(GUID_Lambda guid);
		static bool UnloadMusic(GUID_Lambda guid);

		static bool DecrementTextureMaterialRef(GUID_Lambda guid);

		static GUID_Lambda GetMeshGUID(const String& name);
		static GUID_Lambda GetMaterialGUID(const String& name);
		static GUID_Lambda GetAnimationGUID(const String& name);
		static bool GetAnimationGUIDsFromMeshName(const String& name, TArray<GUID_Lambda>& guids);
		static GUID_Lambda GetTextureGUID(const String& name);
		static GUID_Lambda GetShaderGUID(const String& name);
		static GUID_Lambda GetSoundEffect3DGUID(const String& name);
		static GUID_Lambda GetSoundEffect2DGUID(const String& name);

		static Mesh*				GetMesh(GUID_Lambda guid);
		static Material*			GetMaterial(GUID_Lambda guid);
		static MaterialLoadDesc		GetMaterialDesc(GUID_Lambda guid);
		static Animation*			GetAnimation(GUID_Lambda guid);
		static Texture*				GetTexture(GUID_Lambda guid);
		static TextureView*			GetTextureView(GUID_Lambda guid);
		static Shader*				GetShader(GUID_Lambda guid);
		static ISoundEffect3D*		GetSoundEffect3D(GUID_Lambda guid);
		static ISoundEffect2D*		GetSoundEffect2D(GUID_Lambda guid);
		static IMusic*				GetMusic(GUID_Lambda guid);

		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetMeshNamesMap()			{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetMaterialNamesMap()		{ return s_MaterialNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetAnimationNamesMap()		{ return s_AnimationNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetTextureNamesMap()		{ return s_TextureNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetShaderNamesMap()			{ return s_ShaderNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetSoundEffect3DNamesMap()	{ return s_SoundEffect3DNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetSoundEffect2DNamesMap()	{ return s_SoundEffect2DNamesToGUIDs; }
		FORCEINLINE static std::unordered_map<String, GUID_Lambda>& GetMusicNamesMap()			{ return s_MusicNamesToGUIDs; }

		FORCEINLINE static std::unordered_map<GUID_Lambda, Mesh*>&				GetMeshGUIDMap()			{ return s_Meshes; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Material*>&			GetMaterialGUIDMap()		{ return s_Materials; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Texture*>&			GetTextureGUIDMap()			{ return s_Textures; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, TextureView*>&		GetTextureViewGUIDMap()		{ return s_TextureViews; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, Shader*>&			GetShaderGUIDMap()			{ return s_Shaders; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, ISoundEffect3D*>&	GetSoundEffect3DGUIDMap()	{ return s_SoundEffects3D; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, ISoundEffect2D*>&	GetSoundEffect2DGUIDMap()	{ return s_SoundEffects2D; }
		FORCEINLINE static std::unordered_map<GUID_Lambda, IMusic*>&			GetMusicGUIDMap()			{ return s_Music; }

	private:
		static bool OnShaderRecompileEvent(const ShaderRecompileEvent& event);

		static void RegisterLoadedMaterialTexture(
			LoadedTexture* pLoadedTexture,
			LoadedMaterial* pLoadedMaterial,
			MaterialLoadDesc& materialLoadDescription,
			TArray<TextureView*>& textureViewsToDelete);

		static void RegisterLoadedMaterialTexture(
			LoadedTexture* pLoadedTexture,
			TArray<LoadedMaterial*>& loadedMaterials,
			TArray<MaterialLoadDesc>& materialLoadDescriptions,
			TArray<TextureView*>& textureViewsToDelete);

		static GUID_Lambda RegisterLoadedMaterial(
			const String& name,
			LoadedMaterial* pLoadedMaterial,
			MaterialLoadDesc& materialLoadConfig);

		static GUID_Lambda RegisterMesh(const String& name, Mesh* pMesh);
		static GUID_Lambda RegisterMaterial(const String& name, Material* pMaterial);
		static GUID_Lambda RegisterAnimation(const String& name, Animation* pAnimation);
		static GUID_Lambda RegisterTexture(Texture* pTexture);
		static GUID_Lambda RegisterTextureWithView(Texture* pTexture, TextureView* pTextureView);

		static GUID_Lambda GetGUID(const std::unordered_map<String, GUID_Lambda>& namesToGUIDs, const String& name);

		static void InitMaterialCreation();
		static void InitDefaultResources();

		static void ReleaseMaterialCreation();

	private:
		static GUID_Lambda s_NextFreeGUID;

		static std::unordered_map<GUID_Lambda, String> s_MeshGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_MaterialGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_AnimationGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_TextureGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_ShaderGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_SoundEffect3DGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_SoundEffect2DGUIDsToNames;
		static std::unordered_map<GUID_Lambda, String> s_MusicGUIDsToNames;

		static std::unordered_map<String, GUID_Lambda>			s_MeshNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_MaterialNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_AnimationNamesToGUIDs;
		static std::unordered_map<String, TArray<GUID_Lambda>>	s_FileNamesToAnimationGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_TextureNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_ShaderNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_SoundEffect3DNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_SoundEffect2DNamesToGUIDs;
		static std::unordered_map<String, GUID_Lambda>			s_MusicNamesToGUIDs;

		static std::unordered_map<GUID_Lambda, Mesh*>			s_Meshes;
		static std::unordered_map<GUID_Lambda, Material*>		s_Materials;
		static std::unordered_map<GUID_Lambda, Animation*>		s_Animations;
		static std::unordered_map<GUID_Lambda, Texture*>		s_Textures;
		static std::unordered_map<GUID_Lambda, TextureView*>	s_TextureViews;
		static std::unordered_map<GUID_Lambda, Shader*>			s_Shaders;
		static std::unordered_map<GUID_Lambda, ISoundEffect3D*>	s_SoundEffects3D;
		static std::unordered_map<GUID_Lambda, ISoundEffect2D*>	s_SoundEffects2D;
		static std::unordered_map<GUID_Lambda, IMusic*>			s_Music;

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

		static PipelineState* s_pAllChannelsSeperateMaterialPipelineState;
		static PipelineState* s_pAOSeperateMetRoughCombinedMaterialPipelineState;

		static GUID_Lambda s_AllChannelsSeperateMaterialShaderGUID;
		static GUID_Lambda s_AOSeperateMetRoughCombinedMaterialShaderGUID;

		static TSet<GUID_Lambda> s_UnloadedGUIDs;
	};
}
