#include "Input/API/InputActionSystem.h"
#include "Input/API/Input.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

#include <fstream>


namespace LambdaEngine
{
	THashTable<String, EKey> InputActionSystem::m_CurrentKeyBindings;

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

			String action = m.name.GetString();
			String strKey = m.value.GetString();

			EKey keybinding = StringToKey(strKey);

			if (keybinding == EKey::KEY_UNKNOWN) {
				LOG_ERROR("Action %s is keybounded to unknown. Make sure key is defined.", action.c_str());
			}
			else
			{
				m_CurrentKeyBindings.insert({ action, keybinding });
				LOG_INFO("Action %s is keybounded to %s\n",
					action.c_str(), strKey.c_str());
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

	bool InputActionSystem::ChangeKeyBinding(const String& action, EKey key)
	{
		auto itr = m_CurrentKeyBindings.find(action);

		if (itr != m_CurrentKeyBindings.end()) {
			itr->second = key;
			if (s_ConfigDocument.HasMember(action.c_str()))
			{
				String keyStr = KeyToString(key);
				s_ConfigDocument[action.c_str()].SetString(keyStr.c_str(), static_cast<rapidjson::SizeType>(strlen(keyStr.c_str())), s_ConfigDocument.GetAllocator());

				LOG_INFO("Action %s has changed keybinding to %s\n",
					action.c_str(), KeyToString(key));
				return true;
			}
		}

		LOG_ERROR("Action %s is not defined.", action.c_str());
		return false;
	}

	bool InputActionSystem::IsActive(const String& action)
	{
		auto itr = m_CurrentKeyBindings.find(action);

		if (itr != m_CurrentKeyBindings.end()) {
			EKey keyPressed = itr->second;
			return Input::IsKeyDown(keyPressed);
		}

		LOG_ERROR("Action %s is not defined.", action.c_str());
		return false;
	}

	EKey InputActionSystem::GetKey(const String& action)
	{
		auto itr = m_CurrentKeyBindings.find(action);

		if (itr != m_CurrentKeyBindings.end()) {
			EKey keyPressed = itr->second;
			return keyPressed;
		}

		LOG_ERROR("Action %s is not defined.", action.c_str());
		return EKey::KEY_UNKNOWN;
	}

}
