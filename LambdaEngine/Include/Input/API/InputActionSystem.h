#pragma once
#include "Input/API/InputState.h"
#include "Containers/THashTable.h"
#include "ActionCodes.h"

#pragma warning( push, 0 )
#include <rapidjson/document.h>
#pragma warning( pop )

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
		static bool IsActive(EAction action);
		static bool IsBoundToKey(EAction action);
		static bool IsBoundToMouseButton(EAction action);
		static EKey GetKey(EAction action);
		static EMouseButton GetMouseButton(EAction action);

	private:
		static THashTable<EAction, String> m_CurrentBindings;
		static rapidjson::Document s_ConfigDocument;
	};
}
