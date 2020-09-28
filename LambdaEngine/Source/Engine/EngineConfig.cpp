#include "Engine/EngineConfig.h"

#include "Log/Log.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

#include <fstream>

namespace LambdaEngine
{
    rapidjson::Document EngineConfig::s_ConfigDocument = {};
    using namespace rapidjson;

	bool EngineConfig::LoadFromFile()
	{
		const char* pEngineConfigPath = "engine_config.json";

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

    bool EngineConfig::WriteToFile()
    {
        const char* pEngineConfigPath = "engine_config.json";

        FILE* pFile = fopen(pEngineConfigPath, "w");
        if (!pFile)
        {
            LOG_WARNING("Engine config could not be opened: %s", pEngineConfigPath);
            return false;
        }

        char writeBuffer[2048];
        FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));

        PrettyWriter<FileWriteStream> writer(outputStream);
        s_ConfigDocument.Accept(writer);

        fclose(pFile);

        return true;
    }

    bool EngineConfig::GetBoolProperty(const String& propertyName)
    {
        return s_ConfigDocument[propertyName.c_str()].GetBool();
    }

    float EngineConfig::GetFloatProperty(const String& propertyName)
    {
        return s_ConfigDocument[propertyName.c_str()].GetFloat();
    }

    int EngineConfig::GetIntProperty(const String& propertyName)
    {
        return s_ConfigDocument[propertyName.c_str()].GetInt();
    }

    double EngineConfig::GetDoubleProperty(const String& propertyName)
    {
        return s_ConfigDocument[propertyName.c_str()].GetDouble();
    }

    String EngineConfig::GetStringProperty(const String& propertyName)
    {
        return s_ConfigDocument[propertyName.c_str()].GetString();
    }

    TArray<float> EngineConfig::GetFloatArrayProperty(const String& propertyName)
    {
        const Value& arr = s_ConfigDocument[propertyName.c_str()];
        TArray<float> tArr;
        for (auto& itr : arr.GetArray())
            tArr.PushBack(itr.GetFloat());

        return tArr;
    }

    TArray<int> EngineConfig::GetIntArrayProperty(const String& propertyName)
    {
        const Value& arr = s_ConfigDocument[propertyName.c_str()];
        TArray<int> tArr;
        for (auto& itr : arr.GetArray())
            tArr.PushBack(itr.GetInt());

        return tArr;
    }

    bool EngineConfig::SetBoolProperty(const String& propertyName, const bool value)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            s_ConfigDocument[propertyName.c_str()].SetBool(value);

            return true;
        }

        return false;
    }

    bool EngineConfig::SetFloatProperty(const String& propertyName, const float value)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            s_ConfigDocument[propertyName.c_str()].SetFloat(value);

            return true;
        }
        return false;
    }

    bool EngineConfig::SetIntProperty(const String& propertyName, const int value)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            s_ConfigDocument[propertyName.c_str()].SetFloat(value);

            return true;
        }

        return false;
    }

    bool EngineConfig::SetDoubleProperty(const String& propertyName, const double value)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            s_ConfigDocument[propertyName.c_str()].SetDouble(value);

            return true;
        }
        return false;
    }

    bool EngineConfig::SetStringProperty(const String& propertyName, const String& string)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            s_ConfigDocument[propertyName.c_str()].SetString(string.c_str(), static_cast<SizeType>(strlen(string.c_str())), s_ConfigDocument.GetAllocator());

            return true;
        }
        return false;
    }

    bool EngineConfig::SetFloatArrayProperty(const String& propertyName, const TArray<float>& arr)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

            auto& newArr = s_ConfigDocument[propertyName.c_str()].SetArray();
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

    bool EngineConfig::SetIntArrayProperty(const String& propertyName, const TArray<int>& arr)
    {
        if (s_ConfigDocument.HasMember(propertyName.c_str()))
        {
            Document::AllocatorType& allocator = s_ConfigDocument.GetAllocator();

            auto& newArr = s_ConfigDocument[propertyName.c_str()].SetArray();
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
