#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include <filesystem>

namespace LambdaEngine
{
	FORCEINLINE TArray<String> EnumerateFilesInDirectory(const String& filepath, bool skipDirectories)
	{
		TArray<String> result;

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(filepath))
		{
			if ((skipDirectories && !entry.is_directory()) || !skipDirectories)
				result.push_back(entry.path().filename().string());
		}

		return result;
	}
}