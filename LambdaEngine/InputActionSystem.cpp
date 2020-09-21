#include "PreCompiled.h"
#include "InputActionSystem.h"


namespace LambdaEngine
{
	InputActionSystem::InputActionSystem() {
		m_KeyMapping = { 
			{EAction::PLAYER_FORWARD, EKey::KEY_W},
			{EAction::PLAYER_BACKWARD, EKey::KEY_S},
			{EAction::PLAYER_LEFT, EKey::KEY_A},
			{EAction::PLAYER_RIGHT, EKey::KEY_D},
		};
	}


	bool InputActionSystem::Match(EAction action, EKey key) const
	{
		return m_KeyMapping.at(action) == key;
	}

}