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

        static bool GetBoolProperty(const String& propertyName);

    private:
        static rapidjson::Document s_ConfigDocument;
    };
}
