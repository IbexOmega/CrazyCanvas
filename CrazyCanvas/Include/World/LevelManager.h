#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Math/Math.h"

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

		union
		{
			byte SHA256[256];

			struct
			{
				uint32 SHA256Chunk0;
				uint32 SHA256Chunk1;
				uint32 SHA256Chunk2;
				uint32 SHA256Chunk3;
			};
		};
	};

public:
	DECL_STATIC_CLASS(LevelManager);

	static bool Init();

	static const LambdaEngine::TArray<LambdaEngine::String>& GetLevelNames();
	static void LoadLevel(const LambdaEngine::String& levelName);
	static void LoadLevel(uint32 index);

private:
	static LambdaEngine::TArray<LambdaEngine::String> s_LevelNames;
	static LambdaEngine::TArray<LevelDesc> s_LevelDescriptions;
};