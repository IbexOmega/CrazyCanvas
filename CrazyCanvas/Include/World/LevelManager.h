#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Utilities/SHA256.h"

#include "Math/Math.h"

#include "LevelModule.h"

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
		LambdaEngine::TArray<ModuleDesc>	LevelModules;
		LambdaEngine::SHA256Hash			Hash;
	};

public:
	DECL_STATIC_CLASS(LevelManager);

	static bool Init(bool clientSide);
	static bool Release();
	
	static bool LoadLevel(const LambdaEngine::SHA256Hash& levelHash);
	static bool LoadLevel(const LambdaEngine::String& levelName);
	static bool LoadLevel(uint32 index);

	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& GetLevelHashes() { return s_LevelHashes; }
	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::String>& GetLevelNames() { return s_LevelNames; }
private:
	static inline LambdaEngine::TArray<LambdaEngine::SHA256Hash> s_LevelHashes;
	static inline LambdaEngine::TArray<LambdaEngine::String> s_LevelNames;
	static inline LambdaEngine::TArray<LevelDesc> s_LevelDescriptions;

	static inline LambdaEngine::THashTable<LambdaEngine::String, LevelModule*> s_LoadedModules;
};