#pragma once

#include "LambdaEngine.h"

enum class EGameMode : uint8
{
	NONE			= 0,
	CTF_COMMON_FLAG	= 1,
	CTF_TEAM_FLAG	= 2
};

FORCEINLINE EGameMode ConvertGameMode(uint8 gameModeIndex)
{
	switch (gameModeIndex)
	{
	case 1: return EGameMode::CTF_COMMON_FLAG;
	case 2: return EGameMode::CTF_TEAM_FLAG;
	}

	return EGameMode::NONE;
}

FORCEINLINE bool GameModeRequiresFlag(EGameMode gameMode)
{
	switch (gameMode)
	{
	case EGameMode::NONE:				return false;
	case EGameMode::CTF_COMMON_FLAG:	return true;
	case EGameMode::CTF_TEAM_FLAG:		return true;
	}

	return false;
}