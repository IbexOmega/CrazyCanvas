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
        static TArray<int> GetArrayProperty(const String& propertyName);

        static bool SetBoolProperty(const String& propertyName, bool value);
        static bool SetFloatProperty(const String& propertyName, float value);
        static bool SetIntProperty(const String& propertyName, int value);
        static bool SetArrProperty(const String& propertName, int* arr);

    private:
        static rapidjson::Document s_ConfigDocument;
    };
}
