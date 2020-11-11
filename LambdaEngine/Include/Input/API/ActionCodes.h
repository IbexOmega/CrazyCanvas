#pragma once
#include "LambdaEngine.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	/*
	* EAction
	*/
	enum EAction : uint8
	{
		ACTION_UNKNOWN = 0,

		// Movement
		ACTION_MOVE_FORWARD		= 1,
		ACTION_MOVE_BACKWARD	= 2,
		ACTION_MOVE_RIGHT		= 3,
		ACTION_MOVE_LEFT		= 4,
		ACTION_MOVE_SPRINT		= 5,
		ACTION_MOVE_CROUCH		= 6,
		ACTION_MOVE_JUMP		= 7,

		// Attack
		ACTION_ATTACK_PRIMARY	= 8,
		ACTION_ATTACK_SECONDARY	= 9,
		ACTION_ATTACK_RELOAD	= 10,
		ACTION_ATTACK_MELEE		= 11,

		// General
		ACTION_GENERAL_SCOREBOARD = 12,

		// Debug action codes
		ACTION_CAM_ROT_UP		= 13,
		ACTION_CAM_ROT_DOWN		= 14,
		ACTION_CAM_ROT_LEFT		= 15,
		ACTION_CAM_ROT_RIGHT	= 16,
		ACTION_CAM_UP			= 17,
		ACTION_CAM_DOWN			= 18,
		ACTION_TOGGLE_MOUSE		= 19,
	};

	/*
	* Helpers
	*/
	inline const char* ActionToString(EAction action)
	{
		switch (action)
		{
		// Movement
		case ACTION_MOVE_FORWARD:		return "ACTION_MOVE_FORWARD";
		case ACTION_MOVE_BACKWARD:		return "ACTION_MOVE_BACKWARD";
		case ACTION_MOVE_RIGHT:			return "ACTION_MOVE_RIGHT";
		case ACTION_MOVE_LEFT:			return "ACTION_MOVE_LEFT";
		case ACTION_MOVE_SPRINT:		return "ACTION_MOVE_SPRINT";
		case ACTION_MOVE_CROUCH:		return "ACTION_MOVE_CROUCH";
		case ACTION_MOVE_JUMP:			return "ACTION_MOVE_JUMP";

		// Attack
		case ACTION_ATTACK_PRIMARY:		return "ACTION_ATTACK_PRIMARY";
		case ACTION_ATTACK_SECONDARY:	return "ACTION_ATTACK_SECONDARY";
		case ACTION_ATTACK_RELOAD:		return "ACTION_ATTACK_RELOAD";
		case ACTION_ATTACK_MELEE:		return "ACTION_ATTACK_MELEE";

		// General
		case ACTION_GENERAL_SCOREBOARD:	return "ACTION_GENERAL_SCOREBOARD";

		// Debug action codes
		case ACTION_CAM_ROT_UP:			return "ACTION_CAM_ROT_UP";
		case ACTION_CAM_ROT_DOWN:		return "ACTION_CAM_ROT_DOWN";
		case ACTION_CAM_ROT_LEFT:		return "ACTION_CAM_ROT_LEFT";
		case ACTION_CAM_ROT_RIGHT:		return "ACTION_CAM_ROT_RIGHT";
		case ACTION_CAM_UP:				return "ACTION_CAM_UP";
		case ACTION_CAM_DOWN:			return "ACTION_CAM_UP";
		case ACTION_TOGGLE_MOUSE:		return "ACTION_TOGGLE_MOUSE";

		default:						return "ACTION_UNKNOWN";
		}
	}

	inline const EAction StringToAction(String str)
	{
		static const THashTable<String, EAction> actionMap = {
			{"ACTION_MOVE_FORWARD", EAction::ACTION_MOVE_FORWARD},
			{"ACTION_MOVE_BACKWARD", EAction::ACTION_MOVE_BACKWARD},
			{"ACTION_MOVE_RIGHT", EAction::ACTION_MOVE_RIGHT},
			{"ACTION_MOVE_LEFT", EAction::ACTION_MOVE_LEFT},
			{"ACTION_MOVE_SPRINT", EAction::ACTION_MOVE_SPRINT},
			{"ACTION_MOVE_CROUCH", EAction::ACTION_MOVE_CROUCH},
			{"ACTION_MOVE_JUMP", EAction::ACTION_MOVE_JUMP},
			{"ACTION_ATTACK_PRIMARY", EAction::ACTION_ATTACK_PRIMARY},
			{"ACTION_ATTACK_SECONDARY", EAction::ACTION_ATTACK_SECONDARY},
			{"ACTION_ATTACK_RELOAD", EAction::ACTION_ATTACK_RELOAD},
			{"ACTION_ATTACK_MELEE", EAction::ACTION_ATTACK_MELEE},
			{"ACTION_GENERAL_SCOREBOARD", EAction::ACTION_GENERAL_SCOREBOARD},
			{"ACTION_CAM_ROT_UP", EAction::ACTION_CAM_ROT_UP},
			{"ACTION_CAM_ROT_DOWN", EAction::ACTION_CAM_ROT_DOWN},
			{"ACTION_CAM_ROT_LEFT", EAction::ACTION_CAM_ROT_LEFT},
			{"ACTION_CAM_ROT_RIGHT", EAction::ACTION_CAM_ROT_RIGHT},
			{"ACTION_CAM_UP", EAction::ACTION_CAM_UP},
			{"ACTION_CAM_DOWN", EAction::ACTION_CAM_DOWN},
			{"ACTION_TOGGLE_MOUSE", EAction::ACTION_TOGGLE_MOUSE},
		};

		auto itr = actionMap.find(str);
		if (itr != actionMap.end()) {
			return itr->second;
		}
		return EAction::ACTION_UNKNOWN;
	}
}