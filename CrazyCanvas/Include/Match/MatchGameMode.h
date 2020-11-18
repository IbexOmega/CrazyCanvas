#pragma once

#include "LambdaEngine.h"

enum class EGameMode : uint8
{
	NONE			= 0,
	CTF_COMMON_FLAG	= 1,
	CTF_TEAM_FLAG	= 2
};

constexpr FORCEINLINE uint8 ConvertGameMode(EGameMode gameMode)
{
	switch (gameMode)
	{
	case EGameMode::NONE:				return 0;
	case EGameMode::CTF_COMMON_FLAG:	return 1;
	case EGameMode::CTF_TEAM_FLAG:		return 2;
	}

	return 0;
}

constexpr FORCEINLINE EGameMode ConvertGameMode(uint8 gameModeIndex)
{
	switch (gameModeIndex)
	{
	case 1: return EGameMode::CTF_COMMON_FLAG;
	case 2: return EGameMode::CTF_TEAM_FLAG;
	}

	return EGameMode::NONE;
}

constexpr FORCEINLINE bool GameModeRequiresFlag(EGameMode gameMode)
{
	switch (gameMode)
	{
	case EGameMode::NONE:				return false;
	case EGameMode::CTF_COMMON_FLAG:	return true;
	case EGameMode::CTF_TEAM_FLAG:		return true;
	}

	return false;
}

constexpr FORCEINLINE const char* GameModeToString(EGameMode gameMode)
{
	switch (gameMode)
	{
	case EGameMode::CTF_COMMON_FLAG:	return "One Flag CTF";
	case EGameMode::CTF_TEAM_FLAG:		return "Multi Flag CTF";
	default: return "NONE";
	}
}

FORCEINLINE EGameMode GameModeParseString(const char* gameMode)
{
	if(strcmp(gameMode, "One Flag CTF") == 0)
		return EGameMode::CTF_COMMON_FLAG;
	else if (strcmp(gameMode, "Multi Flag CTF") == 0)
		return EGameMode::CTF_TEAM_FLAG;
	else
		return EGameMode::NONE;
}

FORCEINLINE void GameModesQuery(LambdaEngine::TArray<EGameMode>& gameModes)
{
	gameModes.PushBack(EGameMode::CTF_COMMON_FLAG);
	gameModes.PushBack(EGameMode::CTF_TEAM_FLAG);
}