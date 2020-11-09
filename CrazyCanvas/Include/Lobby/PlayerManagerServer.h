#pragma once
#include "Lobby/PlayerManagerBase.h"
#include "Lobby/IPlayerManager.h"

#include "Time/API/Timestamp.h"

class PlayerManagerServer : public PlayerManagerBase
{
	friend class CrazyCanvas;
public:
	DECL_STATIC_CLASS(PlayerManagerServer);

	static bool HasPlayerAuthority(const LambdaEngine::IClient* pClient);
	static void SetPlayerStateLoading();

protected:
	static void Init();
	static void Release();
	static void FixedTick(LambdaEngine::Timestamp deltaTime);

	static bool OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event);
	static bool OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event);
	static bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	static bool OnPacketPlayerInfoReceived(const PacketReceivedEvent<PacketPlayerInfo>& event);

private:
	static void HandlePlayerLeftServer(LambdaEngine::IClient* pClient);

protected:
	static LambdaEngine::Timestamp s_Timer;
};