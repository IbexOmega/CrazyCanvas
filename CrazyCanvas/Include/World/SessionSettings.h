#pragma once

#include "Events/PacketEvents.h"
#include "Multiplayer/Packet/PacketSessionSettingChanged.h"
#include "SessionSettingsTypes.h"

class SessionSettings
{
public:
	DECL_STATIC_CLASS(SessionSettings);

	static void Init();
	static void Release();

	/**
	 * Sets the setting with the given type. Note: Valid types are expressed in the SettingValue typedef.
	 * @param setting - Setting to set
	 * @param value - Variant value to be set - see typedef to see which types are supported
	**/
	FORCEINLINE static void SetSetting(ESessionSetting setting, SettingValue value)
	{
		if (setting == ESessionSetting::RESET)
		{
			ResetSettings();
		}
		else
		{
			s_CurrentSettings[setting] = value;
		}
	}

	/**
	 * Get the setting using the variant value.
	 * @param setting - Setting to get
	 * @return Variant value that contains the setting. If setting has not been set, returns default value.
	**/
	FORCEINLINE static SettingValue GetSetting(ESessionSetting setting)
	{
		if (s_CurrentSettings.contains(setting))
			return s_CurrentSettings[setting];
		return s_DefaultSettings[setting];
	}

	/**
	 * Get the setting with the desired type. Note: Type must match the previously set value!
	 * @param setting - Setting to get
	 * @return Type value that is the setting value. If setting has not been set, returns default value.
	 * If Type given is incorrect a default constructued Type will be return and error will be printed.
	**/
	template <typename Type>
	FORCEINLINE static Type GetSettingValue(ESessionSetting setting)
	{
		if (!s_CurrentSettings.contains(setting))
		{
			return GetDefaultSettingValue<Type>(setting);
		}

		if (auto pVal = std::get_if<Type>(&s_CurrentSettings[setting]))
		{
			return *pVal;
		}
		else
		{
			LOG_ERROR("Failed to get setting!");
			return Type();
		}
	}

	/**
	 * Get the setting using the variant value.
	 * @param setting - Setting to get
	 * @return Variant value that contains the setting. If setting has not been set, returns default value.
	**/
	FORCEINLINE static SettingValue GetDefaultSetting(ESessionSetting setting)
	{
		return s_DefaultSettings[setting];
	}

	/**
	 * Get the setting with the desired type. Note: Type must match the previously set value!
	 * @param setting - Setting to get
	 * @return Type value that is the setting value. If setting has not been set, returns default value.
	 * If Type given is incorrect a default constructued Type will be return and error will be printed.
	**/
	template <typename Type>
	FORCEINLINE static Type GetDefaultSettingValue(ESessionSetting setting)
	{
		if (auto pVal = std::get_if<Type>(&s_DefaultSettings[setting]))
		{
			return *pVal;
		}
		else
		{
			LOG_ERROR("Failed to get setting!");
			return Type();
		}
	}

	FORCEINLINE static void ResetSettings()
	{
		for (auto settingPair : s_DefaultSettings)
		{
			s_CurrentSettings[settingPair.first] = settingPair.second;
		}
	}

private:
	static bool OnPacketSessionSettingsReceived(const PacketReceivedEvent<PacketSessionSettingChanged>& packetEvent);
	static void SendPacketSessionSettings(ESessionSetting setting, SettingValue value);

private:
	inline static LambdaEngine::THashTable<ESessionSetting, SettingValue> s_CurrentSettings;

	// Default settings
	inline static LambdaEngine::THashTable<ESessionSetting, SettingValue> s_DefaultSettings = {
		{ESessionSetting::GROUND_ACCELERATION,	100.f},
		{ESessionSetting::GROUND_FRICTION,		15.f},
		{ESessionSetting::AIR_ACCELERATION,		4.f},
		{ESessionSetting::MAX_RUN_VELOCITY,		5.f},
		{ESessionSetting::MAX_WALK_VELOCITY,	2.5f},
		{ESessionSetting::MAX_AIR_VELOCITY,		5.f},
		{ESessionSetting::JUMP_SPEED,			5.f},
	};
};