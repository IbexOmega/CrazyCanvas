#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Utilities/SHA256.h"

#include "Math/Math.h"

#include "World.h"

class Level;
class LevelModule;

class LevelManager
{
	struct ModuleDesc
	{
		LambdaEngine::String	Filename		= "";
		glm::vec3				Translation		= glm::vec3(0.0f);
	};

	struct LevelDesc
	{
		LambdaEngine::String				Name = "";
		LambdaEngine::TArray<ModuleDesc>	LevelModuleDescriptions;
		LambdaEngine::SHA256Hash			Hash;
	};

public:
	DECL_STATIC_CLASS(LevelManager);

	static bool Init();
	static bool Release();
	
	/*
	*	Loads a level, the level can then be used to create new entities which are tied to the level
	*	The caller is responsible of deleting the Level
	*/
	static Level* LoadLevel(const LambdaEngine::SHA256Hash& levelHash);
	static Level* LoadLevel(const LambdaEngine::String& levelName);
	static Level* LoadLevel(uint32 index);

	static const LambdaEngine::SHA256Hash& GetHash(const LambdaEngine::String& levelName);
	static const LambdaEngine::SHA256Hash& GetHash(uint32 index);

	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& GetLevelHashes() { return s_LevelHashes; }
	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::String>& GetLevelNames() { return s_LevelNames; }

private:
	static inline LambdaEngine::TArray<LambdaEngine::SHA256Hash> s_LevelHashes;
	static inline LambdaEngine::TArray<LambdaEngine::String> s_LevelNames;
	static inline LambdaEngine::TArray<LevelDesc> s_LevelDescriptions;

	static inline LambdaEngine::THashTable<LambdaEngine::String, LevelModule*> s_LoadedModules;
	static inline LambdaEngine::SHA256Hash s_DefaultEmptyHash;
};