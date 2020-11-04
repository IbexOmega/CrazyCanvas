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
	THashTable<EAction, String> InputActionSystem::m_CurrentBindings;

	rapidjson::Document InputActionSystem::s_ConfigDocument;

	bool InputActionSystem::LoadFromFile()
	{
		const char* pKeyBindingsConfigPath = "key_bindings.json";

		FILE* pFile = fopen(pKeyBindingsConfigPath, "r");
		if (!pFile)
		{
			// TODO: We should probably create a default so that the user does not need to have a config file to even run the application
			LOG_ERROR("Config file could not be opened: %s", pKeyBindingsConfigPath);
			DEBUGBREAK();
			return false;
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
				m_CurrentBindings.insert({ action, strBinding });
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
		if (m_CurrentBindings.contains(action))
		{
			String keyStr = KeyToString(key);
			m_CurrentBindings[action] = keyStr;
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
		if (m_CurrentBindings.contains(action))
		{
			String buttonStr = ButtonToString(button);
			m_CurrentBindings[action] = buttonStr;
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
		if (m_CurrentBindings.contains(action))
		{
			EKey key = StringToKey(m_CurrentBindings[action]);
			EMouseButton mouseButton = StringToButton(m_CurrentBindings[action]);

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
		if (m_CurrentBindings.contains(action))
		{
			return StringToKey(m_CurrentBindings[action]);
		}

		LOG_WARNING("Action %s is not defined.", ActionToString(action));
		return EKey::KEY_UNKNOWN;
	}

	EMouseButton InputActionSystem::GetMouseButton(EAction action)
	{
		if (m_CurrentBindings.contains(action))
		{
			return StringToButton(m_CurrentBindings[action]);
		}

		LOG_WARNING("Action %s is not defined.", ActionToString(action));
		return EMouseButton::MOUSE_BUTTON_UNKNOWN;
	}
}
