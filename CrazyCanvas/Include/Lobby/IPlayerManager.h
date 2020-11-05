#pragma once

#include "Types.h"

#include "Lobby/Player.h"

#include "Networking/API/IClient.h"

class IPlayerManager
{
public:
	DECL_INTERFACE(IPlayerManager);

	/*virtual void HandlePlayerJoined(LambdaEngine::IClient* pClient, const Player& player) = 0;
	virtual void HandlePlayerLeft(LambdaEngine::IClient* pClientToExclude, uint64 uid) = 0;*/
};
