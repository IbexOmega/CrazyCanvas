#pragma once

enum class EPaintMode : uint32
{
	REMOVE	= 0,
	PAINT	= 1,
	NONE	= 2
};

enum class ERemoteMode : uint32
{
	UNDEFINED 	= 0,
	CLIENT		= 1,
	SERVER		= 2
};

enum class ETeam : uint32
{
	NONE	= 0,
	TEAM_1	= 1,
	TEAM_2	= 2
};