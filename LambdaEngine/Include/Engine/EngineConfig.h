#pragma once

#include "Defines.h"
#include "Containers/String.h"
#include "ConfigCodes.h"

#include <argh/argh.h>
#pragma warning( push, 0 )
#include <rapidjson/document.h>
#pragma warning( pop )

namespace LambdaEngine
{
	class LAMBDA_API EngineConfig
	{
	public:
		DECL_STATIC_CLASS(EngineConfig);

		static bool				LoadFromFile(const argh::parser& flagParser);
		static bool				WriteToFile();

		static bool				GetBoolProperty(EConfigOption configOption);
		static float			GetFloatProperty(EConfigOption configOption);
		static int				GetIntProperty(EConfigOption configOption);
		static uint32			GetUint32Property(EConfigOption configOption);
		static double			GetDoubleProperty(EConfigOption configOption);
		static String			GetStringProperty(EConfigOption configOption);
		static TArray<float>	GetFloatArrayProperty(EConfigOption configOption);
		static TArray<int>		GetIntArrayProperty(EConfigOption configOption);

		static bool				SetBoolProperty(EConfigOption configOption, bool value);
		static bool				SetFloatProperty(EConfigOption configOption, float value);
		static bool				SetIntProperty(EConfigOption configOption, int value);
		static bool				SetUint32Property(EConfigOption configOption, uint32 value);
		static bool				SetDoubleProperty(EConfigOption configOption, double value);
		static bool				SetStringProperty(EConfigOption configOption, const String& string);
		static bool				SetFloatArrayProperty(EConfigOption configOption, const TArray<float>& arr);
		static bool				SetIntArrayProperty(EConfigOption configOption, const TArray<int>& arr);

	private:
		static rapidjson::Document s_ConfigDocument;
		static String s_FilePath;
	};
}
