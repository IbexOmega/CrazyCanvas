#pragma once
#include "Input/API/InputState.h"
#include "Containers/THashTable.h"
#include <rapidjson/document.h>

namespace LambdaEngine
{

	class LAMBDA_API InputActionSystem
	{
	public:
		DECL_STATIC_CLASS(InputActionSystem);

		static bool LoadFromFile();
		static bool WriteToFile();
		static bool ChangeKeyBinding(String action, EKey key);
		static bool IsActive(String action);

	private:
		static THashTable<String, EKey> m_CurrentKeyBindings;
		static rapidjson::Document s_ConfigDocument;
	};
}
