#pragma once

#include <variant>

enum class ESessionSetting : uint8
{
	RESET				= 0,
	GROUND_ACCELERATION	= 1,
	GROUND_FRICTION		= 2,
	AIR_ACCELERATION	= 3,
	MAX_RUN_VELOCITY	= 4,
	MAX_WALK_VELOCITY	= 5,
	MAX_AIR_VELOCITY	= 6,
	JUMP_SPEED			= 7,
};

typedef std::variant<int, uint32, float, bool> SettingValue;