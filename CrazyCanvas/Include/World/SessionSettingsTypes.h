#pragma once

#include <variant>

enum class ESessionSetting : uint8
{
	RESET				= 0,
	GROUND_ACCELERATION	= 1,
	GROUND_FRICTION		= 2,
	AIR_ACCELERATION	= 3,
};

typedef std::variant<int, uint32, float, bool> SettingValue;