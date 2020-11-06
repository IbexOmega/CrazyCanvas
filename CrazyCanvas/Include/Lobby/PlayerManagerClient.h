#pragma once

#include "Lobby/IPlayerManager.h"
#include "Lobby/PlayerManagerBase.h"

class PlayerManagerClient : public PlayerManagerBase
{
	friend class CrazyCanvas;
public:
	DECL_UNIQUE_CLASS(PlayerManagerClient);

protected:

public:
	static const Player* GetPlayerLocal();

	static void RegisterLocalPlayer(const LambdaEngine::String& name);

	static void SetLocalPlayerReady(bool ready);

private:
	static void Init();
	static void Release();

	static Player* GetPlayerLocalNoConst();

	static bool OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event);
	static bool OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event);
	static bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	static bool OnPacketPlayerInfoReceived(const PacketReceivedEvent<PacketPlayerInfo>& event);

private:
};