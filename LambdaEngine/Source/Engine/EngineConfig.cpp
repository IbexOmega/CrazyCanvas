#include "Engine/EngineConfig.h"

#include "Log/Log.h"

#pragma warning( push, 0 )
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#pragma warning( pop )

#include <fstream>

namespace LambdaEngine
{
	rapidjson::Document EngineConfig::s_ConfigDocument = {};
	String EngineConfig::s_FilePath;
	using namespace rapidjson;

	bool EngineConfig::LoadFromFile(const argh::parser& flagParser)
	{
		/*	Production-released products will likely not be started with any command line arguments,
			that is when the production config is used */
		constexpr const char* pDefaultPathPostfix = "crazycanvas";
		String pathPostfix;
		flagParser({ "--state" }, pDefaultPathPostfix) >> pathPostfix;
		s_FilePath = "engine_config_" + String(pathPostfix) + ".json";

		FILE* pFile = fopen(s_FilePath.c_str(), "r");
		if (!pFile)
		{
			// The state does not have its own config file, try the default config path
			s_FilePath = "engine_config_" + String(pDefaultPathPostfix) + ".json";

			pFile = fopen(s_FilePath.c_str(), "r");
			if (!pFile)
			{
				// TODO: We should probably create a default so that the user does not need to have a config file to even run the application
				LOG_WARNING("Engine config could not be opened: %s", s_FilePath.c_str());
				DEBUGBREAK();
				return false;
			}
		}

		char readBuffer[2048];
		FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_ConfigDocument.ParseStream(inputStream);

		return fclose(pFile) == 0;
	}

	bool EngineConfig::WriteToFile()
	{
		FILE* pFile = fopen(s_FilePath.c_str(), "w");
		if (!pFile)
		{
			LOG_WARNING("Engine config could not be opened: %s", s_FilePath.c_str());
			return false;
		}

		char writeBuffer[2048];
		FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));

		PrettyWriter<FileWriteStream> writer(outputStream);
		s_ConfigDocument.Accept(writer);

		return fclose(pFile) == 0;
	}

	bool EngineConfig::GetBoolProperty(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetBool();
	}

	float EngineConfig::GetFloatProperty(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetFloat();
	}

	int EngineConfig::GetIntProperty(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetInt();
	}

	uint32 EngineConfig::GetUint32Property(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetUint();
	}

	double EngineConfig::GetDoubleProperty(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetDouble();
	}

	String EngineConfig::GetStringProperty(EConfigOption configOption)
	{
		return s_ConfigDocument[ConfigOptionToString(configOption)].GetString();
	}

	TArray<float> EngineConfig::GetFloatArrayProperty(EConfigOption configOption)
	{
		const Value& arr = s_ConfigDocument[ConfigOptionToString(configOption)];
		TArray<float> tArr;
		for (auto& itr : arr.GetArray())
			tArr.PushBack(itr.GetFloat());

		return tArr;
	}

	TArray<int> EngineConfig::GetIntArrayProperty(EConfigOption configOption)
	{
		const Value& arr = s_ConfigDocument[ConfigOptionToString(configOption)];
		TArray<int> tArr;
		for (auto& itr : arr.GetArray())
			tArr.PushBack(itr.GetInt());

		return tArr;
	}

	bool EngineConfig::SetBoolProperty(EConfigOption configOption, const bool value)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetBool(value);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetFloatProperty(EConfigOption configOption, const float value)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetFloat(value);

			return true;
		}
		return false;
	}

	bool EngineConfig::SetIntProperty(EConfigOption configOption, const int value)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetInt(value);

			return true;
		}

		return false;
	}

		bool EngineConfig::SetUint32Property(EConfigOption configOption, const uint32 value)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetUint(value);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetDoubleProperty(EConfigOption configOption, const double value)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetDouble(value);

			return true;
		}
		return false;
	}

	bool EngineConfig::SetStringProperty(EConfigOption configOption, const String& string)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			s_ConfigDocument[ConfigOptionToString(configOption)].SetString(string.c_str(), static_cast<SizeType>(strlen(string.c_str())), s_ConfigDocument.GetAllocator());

			return true;
		}
		return false;
	}

	bool EngineConfig::SetFloatArrayProperty(EConfigOption configOption, const TArray<float>& arr)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

			auto& newArr = s_ConfigDocument[ConfigOptionToString(configOption)].SetArray();
			newArr.Reserve(arr.GetSize(), allocator);

			for (auto& itr : arr)
				newArr.PushBack(Value().SetFloat(itr), allocator);

			StringBuffer strBuf;
			PrettyWriter<StringBuffer> writer(strBuf);
			s_ConfigDocument.Accept(writer);

			return true;
		}

		return false;
	}

	bool EngineConfig::SetIntArrayProperty(EConfigOption configOption, const TArray<int>& arr)
	{
		if (s_ConfigDocument.HasMember(ConfigOptionToString(configOption)))
		{
			Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

			auto& newArr = s_ConfigDocument[ConfigOptionToString(configOption)].SetArray();
			newArr.Reserve(arr.GetSize(), allocator);

			for (auto& itr : arr)
				newArr.PushBack(Value().SetInt(itr), allocator);

			StringBuffer strBuf;
			PrettyWriter<StringBuffer> writer(strBuf);
			s_ConfigDocument.Accept(writer);

			return true;
		}

		return false;
	}
}
