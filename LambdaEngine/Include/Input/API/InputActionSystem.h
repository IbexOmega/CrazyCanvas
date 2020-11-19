#pragma once
#include "Input/API/InputState.h"
#include "Containers/THashTable.h"
#include "ActionCodes.h"

#pragma warning( push, 0 )
#include <rapidjson/document.h>
#pragma warning( pop )

constexpr const float32 LOOK_SENSITIVITY_BASE = 0.005f;

namespace LambdaEngine
{
	class LAMBDA_API InputActionSystem
	{
	public:
		DECL_STATIC_CLASS(InputActionSystem);

		static bool LoadFromFile();
		static bool WriteToFile();
		static bool ChangeKeyBinding(EAction action, EKey key);
		static bool ChangeKeyBinding(EAction action, EMouseButton button);
		static bool ChangeKeyBinding(EAction action, String keyOrButton);
		static void SetLookSensitivity(float32 sensitivity);
		static bool IsActive(EAction action);
		static bool IsBoundToKey(EAction action);
		static bool IsBoundToMouseButton(EAction action);
		static EKey GetKey(EAction action);
		static EMouseButton GetMouseButton(EAction action);
		
		FORCEINLINE static float32 GetLookSensitivityPercentage() { return s_CurrentLookSensitivityPercentage; }
		FORCEINLINE static float32 GetLookSensitivity() { return s_CurrentLookSensitivity; }

	private:
		static void CreateDefaults();
		static void WriteNewMember(EAction action, String strValue);

	private:
		static THashTable<EAction, String> s_CurrentBindings;
		static rapidjson::Document s_ConfigDocument;
		static THashTable<EAction, String> s_DefaultBindings;

		static String s_LookSensitivityName;
		static float32 s_CurrentLookSensitivityPercentage;
		static float32 s_DefaultLookSensitivityPercentage;
		static float32 s_CurrentLookSensitivity;
	};
}
