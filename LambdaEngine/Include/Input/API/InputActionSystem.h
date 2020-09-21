#pragma once
#include "Input/API/InputState.h"
#include "Containers/THashTable.h"

namespace LambdaEngine
{
	enum EAction {
		PLAYER_FORWARD,
		PLAYER_BACKWARD,
		PLAYER_LEFT,
		PLAYER_RIGHT,
		// more actions to come...
	};

	class LAMBDA_API InputActionSystem
	{
	public:
		DECL_STATIC_CLASS(InputActionSystem);

		// TODO: Read keymapping from file
		static bool IsActive(EAction action);

		FORCEINLINE static InputActionSystem* GetDevice()
		{
			return s_pInputActionSystem;
		}

	private:
		static InputActionSystem* s_pInputActionSystem;
		static std::unordered_map<EAction, EKey> m_KeyMapping;
	};

}
