#pragma once

#include "Lobby/PlayerManagerBase.h"

#include "Multiplayer/Packet/PacketPlayerState.h"
#include "Multiplayer/Packet/PacketPlayerReady.h"
#include "Multiplayer/Packet/PacketPlayerDied.h"
#include "Multiplayer/Packet/PacketPlayerHost.h"
#include "Multiplayer/Packet/PacketPlayerPing.h"

class PlayerManagerClient : public PlayerManagerBase
{
	friend class CrazyCanvas;
public:
	DECL_UNIQUE_CLASS(PlayerManagerClient);

public:
	static const Player* GetPlayerLocal();
	static void RegisterLocalPlayer(const LambdaEngine::String& name, bool isHost);
	static void SetLocalPlayerReady(bool ready);
	static void SetLocalPlayerStateLoading();
	static void SetLocalPlayerStateLoaded();

	static void Reset();

private:
	static void Init();
	static void Release();

	static Player* GetPlayerLocalNoConst();

	static bool OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event);
	static bool OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event);
	static bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	static bool OnPacketPlayerScoreReceived(const PacketReceivedEvent<PacketPlayerScore>& event);
	static bool OnPacketPlayerStateReceived(const PacketReceivedEvent<PacketPlayerState>& event);
	static bool OnPacketPlayerReadyReceived(const PacketReceivedEvent<PacketPlayerReady>& event);
	static bool OnPacketPlayerDiedReceived(const PacketReceivedEvent<PacketPlayerDied>& event);
	static bool OnPacketPlayerHostReceived(const PacketReceivedEvent<PacketPlayerHost>& event);
	static bool OnPacketPlayerPingReceived(const PacketReceivedEvent<PacketPlayerPing>& event);
};