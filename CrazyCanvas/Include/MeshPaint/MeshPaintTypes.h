#pragma once

enum class EPaintMode : uint32
{
	REMOVE	= 0,
	PAINT	= 1,
	NONE	= 2
};
#define PAINT_MODE_TO_STR(m) (m == EPaintMode::PAINT ? "PAINT" : (m == EPaintMode::REMOVE ? "REMOVE" : "NONE"))

enum class ERemoteMode : uint32
{
	UNDEFINED 	= 0,
	CLIENT		= 1,
	SERVER		= 2
};
#define REMOTE_MODE_TO_STR(m) (m == ERemoteMode::CLIENT ? "CLIENT" : (m == ERemoteMode::SERVER ? "SERVER" : "UNDEFINED"))

enum class ETeam : uint32
{
	NONE	= 0,
	TEAM_1	= 1,
	TEAM_2	= 2
};
#define TEAM_TO_STR(m) (m == ETeam::TEAM_1 ? "TEAM_1" : (m == ETeam::TEAM_2 ? "TEAM_2" : "NONE"))