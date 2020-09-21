#pragma once
#include "Input\API\InputState.h"
#include <map>

namespace LambdaEngine
{
	enum EAction {
		PLAYER_FORWARD,
		PLAYER_BACKWARD,
		PLAYER_LEFT,
		PLAYER_RIGHT,
	};

	class InputActionSystem
	{
	public:
		DECL_STATIC_CLASS(InputActionSystem);

		// TODO: Read keymapping from file
		bool Match(EAction action, EKey key) const;

	private:
		std::unordered_map<EAction, EKey> m_KeyMapping;
	};

}