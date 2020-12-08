#pragma once

#include "Containers/TArray.h"

#include "Lobby/ServerInfo.h"

class ServerFileStorage
{
public:
	static bool LoadServers(LambdaEngine::TArray<ServerInfo>& serverInfos, uint16 defaultPort);
	static bool SaveServers(const LambdaEngine::TArray<ServerInfo>& serverInfos);

private:
	static FILE* CreateFile();
};