#include "Engine/EngineConfig.h"

#include "Log/Log.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <fstream>

namespace LambdaEngine
{
    rapidjson::Document EngineConfig::s_ConfigDocument = {};
    using namespace rapidjson;

    bool EngineConfig::LoadFromFile()
    {
        const char* pEngineConfigPath = "../engine_config.json";

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
        const char* pEngineConfigPath = "../engine_config.json";

        FILE* pFile = fopen(pEngineConfigPath, "wb");
        if (!pFile)
        {
            LOG_WARNING("Engine config could not be opened: %s", pEngineConfigPath);
            return false;
        }

        char writeBuffer[2048];
        FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));

        Writer<FileWriteStream> writer(outputStream);
        s_ConfigDocument.Accept(writer);

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

    TArray<int> EngineConfig::GetArrayProperty(const String& propertyName)
    {
        const Value& arr = s_ConfigDocument[propertyName.c_str()];
        TArray<int> tArr;
        for (auto& itr : arr.GetArray())
            tArr.PushBack(itr.GetInt());
        return tArr;
    }

    bool EngineConfig::SetBoolProperty(const String& propertyName, bool value)
    {
        s_ConfigDocument[propertyName.c_str()].SetBool(value);
        return true;
    }

    bool EngineConfig::SetFloatProperty(const String& propertyName, float value)
    {
        s_ConfigDocument[propertyName.c_str()].SetFloat(value);
        return true;
    }

    bool EngineConfig::SetIntProperty(const String& propertyName, int value)
    {
        s_ConfigDocument[propertyName.c_str()].SetFloat(value);
        return true;
    }

    bool EngineConfig::SetArrProperty(const String& propertName, int* arr)
    {
        
        s_ConfigDocument[propertName.c_str()].SetArray(arr);
        return true;
    }


}
