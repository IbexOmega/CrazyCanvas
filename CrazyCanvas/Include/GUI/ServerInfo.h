#pragma once

#include "NsGui/Grid.h"

#include "Time/API/Timestamp.h"

#include <string>

struct ServerInfo
{
	Noesis::Ptr<Noesis::Grid> ServerGrid;

	std::string Name;
	std::string MapName;
	uint8 Players;
	uint16 Ping;
	LambdaEngine::Timestamp LastUpdate;

	bool operator==(const ServerInfo& other) const
	{
		return Name == other.Name && MapName == other.MapName && Players == other.Players && Ping == other.Ping;
	}

	bool operator!=(const ServerInfo& other) const
	{
		return Name != other.Name || MapName != other.MapName || Players != other.Players || Ping != other.Ping;
	}

	ServerInfo& operator=(const ServerInfo& other)
	{
		if (this != &other)
		{
			Name = other.Name;
			MapName = other.MapName;
			Players = other.Players;
			Ping = other.Ping;
			LastUpdate = other.LastUpdate;
		}
		return *this;
	}
};