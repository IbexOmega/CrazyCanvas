#pragma once

#include "Types.h"

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Networking/API/IPEndPoint.h"

struct ServerInfo
{
	LambdaEngine::String Name;
	LambdaEngine::String MapName;

	uint8 Players		= 0;
	uint8 MaxPlayers	= 0;
	uint16 Ping			= UINT16_MAX;
	uint64 ServerUID	= 0;
	bool IsLAN			= false;
	bool IsOnline		= false;

	LambdaEngine::IPEndPoint EndPoint;
	LambdaEngine::Timestamp LastPinged;

	int32 ClientHostID = 0;

	bool operator==(const ServerInfo& other) const
	{
		return
			Name == other.Name &&
			MapName == other.MapName &&
			Players == other.Players &&
			MaxPlayers == other.MaxPlayers &&
			Ping == other.Ping &&
			ServerUID == other.ServerUID &&
			IsLAN == other.IsLAN &&
			IsOnline == other.IsOnline &&
			EndPoint == other.EndPoint;
	}

	bool operator!=(const ServerInfo& other) const
	{
		return
			Name != other.Name ||
			MapName != other.MapName ||
			Players != other.Players ||
			MaxPlayers != other.MaxPlayers ||
			Ping != other.Ping ||
			ServerUID != other.ServerUID ||
			IsLAN != other.IsLAN ||
			IsOnline != other.IsOnline ||
			EndPoint != other.EndPoint;
	}

	ServerInfo& operator=(const ServerInfo& other)
	{
		if (this != &other)
		{
			Name = other.Name;
			MapName = other.MapName;
			Players = other.Players;
			MaxPlayers = other.MaxPlayers;
			Ping = other.Ping;
			ServerUID = other.ServerUID;
			IsLAN = other.IsLAN;
			IsOnline = other.IsOnline;
			EndPoint = other.EndPoint;
			LastPinged = other.LastPinged;
		}
		return *this;
	}
};