#include "Engine/EngineConfig.h"

#include "Log/Log.h"

#include <rapidjson/filereadstream.h>

#include <fstream>

namespace LambdaEngine
{
	rapidjson::Document EngineConfig::s_ConfigDocument = {};

	bool EngineConfig::LoadFromFile()
	{
		const char* pEngineConfigPath = "../engine_config.json";

		using namespace rapidjson;
		FILE* pFile = fopen(pEngineConfigPath, "r");
		if (!pFile)
		{
			LOG_WARNING("Engine config could not be opened: %s", pEngineConfigPath);
			return false;
		}

		char readBuffer[2048];
		FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_ConfigDocument.ParseStream(inputStream);

		fclose(pFile);

		return true;
	}

	bool EngineConfig::GetBoolProperty(const String& propertyName)
	{
		return s_ConfigDocument[propertyName.c_str()].GetBool();
	}
}
