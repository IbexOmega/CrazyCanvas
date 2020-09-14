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

        static bool             LoadFromFile();
        static bool             WriteToFile();

        static bool             GetBoolProperty(const String& propertyName);
        static float            GetFloatProperty(const String& propertyName);
        static int              GetIntProperty(const String& propertyName);
        static double           GetDoubleProperty(const String& propertyName);
        static String           GetStringProperty(const String& propertyName);
        static TArray<float>    GetFloatArrayProperty(const String& propertyName);
        static TArray<int>      GetIntArrayProperty(const String& propertyName);

        static bool             SetBoolProperty(const String& propertyName, bool value);
        static bool             SetFloatProperty(const String& propertyName, float value);
        static bool             SetIntProperty(const String& propertyName, int value);
        static bool             SetDoubleProperty(const String& propertyName, double value);
        static bool             SetStringProperty(const String& propertyName, const String& string);
        static bool             SetFloatArrayProperty(const String& propertyName, const TArray<float>& arr);
        static bool             SetIntArrayProperty(const String& propertyName, const TArray<int>& arr);

	private:
		static rapidjson::Document s_ConfigDocument;
	};
}
