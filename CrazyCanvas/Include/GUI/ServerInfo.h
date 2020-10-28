#pragma once

#include "NsGui/Grid.h"

#include "Time/API/Timestamp.h"
#include "Networking/API/IPEndPoint.h"

#include "Containers/THashTable.h"
#pragma warning( push, 0 )
#include <rapidjson/document.h>
#pragma warning( pop )

#include <string>
#include "Containers/TArray.h"


	struct ServerInfo
	{
		Noesis::Ptr<Noesis::Grid> ServerGrid;

		std::string Name;
		std::string MapName;

		uint8 Players;
		uint16 Ping;
		uint64 ServerUID;

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
				Name = other.Name;
				MapName = other.MapName;
				Players = other.Players;
				Ping = other.Ping;
				LastUpdate = other.LastUpdate;
				EndPoint = other.EndPoint;
				ServerUID = other.ServerUID;
			}
			return *this;
		}
	};


class SavedServerSystem
{
public:

	static bool LoadIpsFromFile(LambdaEngine::TArray<LambdaEngine::String>& ipAddressess);
	static bool WriteIpsToFile(const char* ip);
	static void CreateFile();
	//static bool IsOnline(const String& action);

private:
	static rapidjson::Document s_SavedServerDocument;
};

