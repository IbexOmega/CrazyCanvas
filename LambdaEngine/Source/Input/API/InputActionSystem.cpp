#include "Input/API/InputActionSystem.h"
#include "Input/API/Input.h"

#pragma warning( push, 0 )
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#pragma warning( pop )
#include <fstream>


namespace LambdaEngine
{
	THashTable<EAction, String> InputActionSystem::s_CurrentBindings;
	rapidjson::Document InputActionSystem::s_ConfigDocument;

	THashTable<EAction, String> InputActionSystem::s_DefaultBindings = {
		// Movement
		{EAction::ACTION_MOVE_FORWARD, "W"},
		{EAction::ACTION_MOVE_BACKWARD, "S"},
		{EAction::ACTION_MOVE_LEFT, "A"},
		{EAction::ACTION_MOVE_RIGHT, "D"},
		{EAction::ACTION_MOVE_SPRINT, "LEFT_SHIFT"},
		{EAction::ACTION_MOVE_CROUCH, "LEFT_CONTROL"},
		{EAction::ACTION_MOVE_JUMP, "SPACE"},

		// Attack
		{EAction::ACTION_ATTACK_PRIMARY, "MOUSE_LEFT"},
		{EAction::ACTION_ATTACK_SECONDARY, "MOUSE_RIGHT"},
		{EAction::ACTION_ATTACK_RELOAD, "R"},
		{EAction::ACTION_ATTACK_MELEE, "E"},

		// General
		{EAction::ACTION_GENERAL_SCOREBOARD, "TAB"},

		// Debug
		{EAction::ACTION_CAM_ROT_UP, "UP"},
		{EAction::ACTION_CAM_ROT_DOWN, "DOWN"},
		{EAction::ACTION_CAM_ROT_LEFT, "LEFT"},
		{EAction::ACTION_CAM_ROT_RIGHT, "RIGHT"},
		{EAction::ACTION_CAM_UP, "E"},
		{EAction::ACTION_CAM_DOWN, "Q"},
		{EAction::ACTION_TOGGLE_MOUSE, "C"},
	};

	bool InputActionSystem::LoadFromFile()
	{
		const char* pKeyBindingsConfigPath = "key_bindings.json";

		FILE* pFile = fopen(pKeyBindingsConfigPath, "r");
		if (!pFile)
		{
			LOG_ERROR("Config file could not be opened: %s. \nWill create a default file", pKeyBindingsConfigPath);
			CreateDefaults();
			return true;
		}

		char readBuffer[2048];
		rapidjson::FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

		s_ConfigDocument.ParseStream(inputStream);

		for (auto& m : s_ConfigDocument.GetObject())
		{

			String strBinding 	= m.value.GetString();
			String strAction	= m.name.GetString();
			EAction action		= StringToAction(strAction);

			EKey keybinding = StringToKey(strBinding);
			EMouseButton mouseButtonBinding = StringToButton(strBinding);
			if (keybinding == EKey::KEY_UNKNOWN && mouseButtonBinding == EMouseButton::MOUSE_BUTTON_UNKNOWN)
			{
				LOG_ERROR("Action %s is bounded to unknown. Make sure key is defined.", strAction.c_str());
			}
			else
			{
				s_CurrentBindings.insert({ action, strBinding });
				LOG_INFO("Action %s is bounded to %s\n", strAction.c_str(), strBinding.c_str());
			}

		}

		fclose(pFile);

		return true;
	}

	bool InputActionSystem::WriteToFile()
	{

		const char* pKeyBindingsConfigPath = "key_bindings.json";

		FILE* pFile = fopen(pKeyBindingsConfigPath, "w");
		if (!pFile)
		{
			LOG_WARNING("Config file could not be opened: %s", pKeyBindingsConfigPath);
			return false;
		}

		char writeBuffer[2048];
		rapidjson::FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));

		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(outputStream);
		s_ConfigDocument.Accept(writer);

		fclose(pFile);

		return true;
	}

	bool InputActionSystem::ChangeKeyBinding(EAction action, EKey key)
	{
		String actionStr = ActionToString(action);
		if (s_CurrentBindings.contains(action))
		{
			String keyStr = KeyToString(key);
			s_CurrentBindings[action] = keyStr;
			if (s_ConfigDocument.HasMember(actionStr.c_str()))
			{
				s_ConfigDocument[actionStr.c_str()].SetString(keyStr.c_str(),
					static_cast<rapidjson::SizeType>(keyStr.length()), s_ConfigDocument.GetAllocator());

				WriteToFile();

				LOG_INFO("Action %s has changed keybinding to %s\n",
					actionStr.c_str(), keyStr.c_str());
				return true;
			}
		}

		LOG_WARNING("Action %s is not defined.", actionStr.c_str());
		return false;
	}

	bool InputActionSystem::ChangeKeyBinding(EAction action, EMouseButton button)
	{
		String actionStr = ActionToString(action);
		if (s_CurrentBindings.contains(action))
		{
			String buttonStr = ButtonToString(button);
			s_CurrentBindings[action] = buttonStr;
			if (s_ConfigDocument.HasMember(actionStr.c_str()))
			{
				s_ConfigDocument[actionStr.c_str()].SetString(buttonStr.c_str(),
					static_cast<rapidjson::SizeType>(buttonStr.length()), s_ConfigDocument.GetAllocator());

				WriteToFile();

				LOG_INFO("Action %s has changed keybinding to %s\n",
					actionStr.c_str(), buttonStr.c_str());
				return true;
			}
		}

		LOG_WARNING("Action %s is not defined.", actionStr.c_str());
		return false;
	}

	bool InputActionSystem::ChangeKeyBinding(EAction action, String keyOrButton)
	{
		EKey key = StringToKey(keyOrButton);
		EMouseButton mouseButton = StringToButton(keyOrButton);

		if (key != EKey::KEY_UNKNOWN)
		{
			return ChangeKeyBinding(action, key);
		}
		else if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			return ChangeKeyBinding(action, mouseButton);
		}

		LOG_WARNING("Action %s is not defined.", ActionToString(action));
		return false;
	}


	bool InputActionSystem::IsActive(EAction action)
	{
		if (s_CurrentBindings.contains(action))
		{
			EKey key = StringToKey(s_CurrentBindings[action]);
			EMouseButton mouseButton = StringToButton(s_CurrentBindings[action]);

			if (key != EKey::KEY_UNKNOWN)
			{
				return Input::IsKeyDown(EInputLayer::GAME, key);
			}
			else if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
			{
				return Input::GetMouseState(EInputLayer::GAME).IsButtonPressed(mouseButton);
			}
		}

		LOG_ERROR("Action %s is not defined.", ActionToString(action));
		return false;
	}

	bool InputActionSystem::IsBoundToKey(EAction action)
	{
		return GetKey(action) != EKey::KEY_UNKNOWN;
	}

	bool InputActionSystem::IsBoundToMouseButton(EAction action)
	{
		return GetMouseButton(action) != EMouseButton::MOUSE_BUTTON_UNKNOWN;
	}

	EKey InputActionSystem::GetKey(EAction action)
	{
		if (s_CurrentBindings.contains(action))
		{
			return StringToKey(s_CurrentBindings[action]);
		}
		else if (s_DefaultBindings.contains(action))
		{
			LOG_WARNING("Binding %s was not found. Using default (%s) instead",
				ActionToString(action), s_DefaultBindings[action].c_str());

			s_CurrentBindings[action] = s_DefaultBindings[action];
			WriteNewMember(action, s_CurrentBindings[action]);
			return StringToKey(s_CurrentBindings[action]);
		}

		LOG_WARNING("Action %s is not defined.", ActionToString(action));
		return EKey::KEY_UNKNOWN;
	}

	EMouseButton InputActionSystem::GetMouseButton(EAction action)
	{
		if (s_CurrentBindings.contains(action))
		{
			return StringToButton(s_CurrentBindings[action]);
		}
		else if (s_DefaultBindings.contains(action))
		{
			LOG_WARNING("Binding %s was not found. Using default (%s) instead",
				ActionToString(action), s_DefaultBindings[action].c_str());

			s_CurrentBindings[action] = s_DefaultBindings[action];
			WriteNewMember(action, s_CurrentBindings[action]);
			return StringToButton(s_CurrentBindings[action]);
		}

		LOG_WARNING("Action %s is not defined.", ActionToString(action));
		return EMouseButton::MOUSE_BUTTON_UNKNOWN;
	}

	void InputActionSystem::CreateDefaults()
	{
		s_ConfigDocument.SetObject();
		for (auto& pair : s_DefaultBindings)
		{
			s_CurrentBindings.insert({ pair.first, pair.second });
			rapidjson::Value key(ActionToString(pair.first), static_cast<rapidjson::SizeType>(strlen(ActionToString(pair.first))), s_ConfigDocument.GetAllocator());
			rapidjson::Value value(pair.second.c_str(), static_cast<rapidjson::SizeType>(pair.second.size()), s_ConfigDocument.GetAllocator());
			s_ConfigDocument.AddMember(key, value, s_ConfigDocument.GetAllocator());
		}

		WriteToFile();
	}

	void InputActionSystem::WriteNewMember(EAction action, String strValue)
	{
		rapidjson::Value key(ActionToString(action), static_cast<rapidjson::SizeType>(strlen(ActionToString(action))), s_ConfigDocument.GetAllocator());
		rapidjson::Value value(strValue.c_str(), static_cast<rapidjson::SizeType>(strValue.size()), s_ConfigDocument.GetAllocator());
		s_ConfigDocument.AddMember(key, value, s_ConfigDocument.GetAllocator());

		WriteToFile();
	}
}
