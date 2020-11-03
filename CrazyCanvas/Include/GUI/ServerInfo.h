#pragma once

#include "NsGui/Grid.h"

#include "Time/API/Timestamp.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/IPAddress.h"

#include "Containers/THashTable.h"
#pragma warning( push, 0 )
#include <rapidjson/document.h>
#pragma warning( pop )

#include <string>
#include "Containers/THashTable.h"
#include "Containers/TArray.h"

struct ServerInfo
{
	Noesis::Ptr<Noesis::Grid> GridUI;

	std::string Name;
	std::string MapName;

	uint8 Players		= 0;
	uint16 Ping			= UINT16_MAX;
	uint64 ServerUID	= UINT64_MAX;
	bool IsLAN			= false;
	bool IsOnline		= false;

	LambdaEngine::IPEndPoint EndPoint;
	LambdaEngine::Timestamp LastUpdate;

	bool operator==(const ServerInfo& other) const
	{
		return Name == other.Name && MapName == other.MapName && Players == other.Players && Ping == other.Ping && EndPoint == other.EndPoint && ServerUID == other.ServerUID;
	}

	bool operator!=(const ServerInfo& other) const
	{
		return Name != other.Name || MapName != other.MapName || Players != other.Players || Ping != other.Ping || EndPoint != other.EndPoint || ServerUID != other.ServerUID;
	}

	ServerInfo& operator=(const ServerInfo& other)
	{
		if (this != &other)
		{
			Name		= other.Name;
			MapName		= other.MapName;
			Players		= other.Players;
			Ping		= other.Ping;
			LastUpdate	= other.LastUpdate;
			EndPoint	= other.EndPoint;
			ServerUID	= other.ServerUID;
			IsLAN		= other.IsLAN;
			IsOnline	= other.IsOnline;
		}
		return *this;
	}
};

class SavedServerSystem
{
public:
	static bool LoadServers(LambdaEngine::TArray<ServerInfo>& serverInfos, uint16 defaultPort);
	static bool SaveServers(const LambdaEngine::THashTable<LambdaEngine::IPAddress*, ServerInfo, LambdaEngine::IPAddressHasher>& serverInfos);

private:
	static FILE* CreateFile();
};