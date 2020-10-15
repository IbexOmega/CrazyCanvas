#pragma once

#include "NsGui/Grid.h"

#include "Time/API/Timestamp.h"
#include "Networking/API/IPEndPoint.h"

#include <string>

struct ServerInfo
{
	Noesis::Ptr<Noesis::Grid> ServerGrid;

	std::string Name;
	std::string MapName;

	uint8 Players;
	uint16 Ping;

	LambdaEngine::IPEndPoint EndPoint;
	LambdaEngine::Timestamp LastUpdate;


	bool operator==(const ServerInfo& other) const
	{
		return Name == other.Name && MapName == other.MapName && Players == other.Players && Ping == other.Ping && EndPoint == other.EndPoint;
	}

	bool operator!=(const ServerInfo& other) const
	{
		return Name != other.Name || MapName != other.MapName || Players != other.Players || Ping != other.Ping || EndPoint != other.EndPoint;
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
			EndPoint = other.EndPoint;
		}
		return *this;
	}
};