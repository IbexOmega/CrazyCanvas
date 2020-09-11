#pragma once

#include "Defines.h"
#include "Containers/String.h"

#include <rapidjson/document.h>

namespace LambdaEngine
{
    class LAMBDA_API EngineConfig
    {
    public:
        DECL_STATIC_CLASS(EngineConfig);

        static bool LoadFromFile();
        static bool WriteToFile();

        static bool GetBoolProperty(const String& propertyName);
        static float GetFloatProperty(const String& propertyName);
        static int GetIntProperty(const String& propertyName);
        static String GetStringProperty(const String& propertyName);
        static TArray<int> GetArrayProperty(const String& propertyName);

        static bool SetBoolProperty(const String& propertyName, const bool& value);
        static bool SetFloatProperty(const String& propertyName, const float& value);
        static bool SetIntProperty(const String& propertyName, const int& value);
        static bool SetStringProperty(const String& propertyName, const String& string);
        static bool SetArrayProperty(const String& propertyName, const TArray<int>& arr);

    private:
        static rapidjson::Document s_ConfigDocument;
    };
}
