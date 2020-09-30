#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include <filesystem>

namespace LambdaEngine
{
	struct LambdaDirectory
	{
	public:
		std::filesystem::path			RelativePath;
		bool							isDirectory = false;

		std::vector<LambdaDirectory>	Children;
	};

	inline TArray<String> EnumerateFilesInDirectory(const String& filepath, bool skipDirectories)
	{
		TArray<String> result;

		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(filepath))
		{
			if ((skipDirectories && !entry.is_directory()) || !skipDirectories)
				result.PushBack(entry.path().filename().string());
		}

		return result;
	}

	inline LambdaDirectory ExtractDirectory(const String& filepath, const std::filesystem::path& relativePath, bool ignoreRootPath = true)
	{
		LambdaDirectory result;
		std::filesystem::path rootPath(filepath);

		result.RelativePath /= ignoreRootPath ? relativePath : relativePath / rootPath.stem().string();
		result.isDirectory = true;

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(rootPath))
		{
			if (entry.is_directory())
			{
				LambdaDirectory childDir = ExtractDirectory(entry.path().string(), result.RelativePath, false);
				result.Children.push_back(childDir);
			}
			else
			{
				LambdaDirectory childDir;
				childDir.RelativePath = result.RelativePath / entry.path().filename();
				result.Children.push_back(childDir);
			}
		}

		return result;
	}
}