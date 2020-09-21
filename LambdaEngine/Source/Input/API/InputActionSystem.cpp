#include "Input/API/InputActionSystem.h"
#include "Input/API/Input.h"


namespace LambdaEngine
{
	InputActionSystem* InputActionSystem::s_pInputActionSystem;
	std::unordered_map<EAction, EKey> InputActionSystem::m_KeyMapping = {
			{EAction::PLAYER_FORWARD, EKey::KEY_W},
			{EAction::PLAYER_BACKWARD, EKey::KEY_S},
			{EAction::PLAYER_LEFT, EKey::KEY_A},
			{EAction::PLAYER_RIGHT, EKey::KEY_D},
	};

	bool InputActionSystem::IsActive(EAction action)
	{	
		EKey keyPressed = s_pInputActionSystem->m_KeyMapping.at(action);
		return Input::IsKeyDown(keyPressed);
	}

}