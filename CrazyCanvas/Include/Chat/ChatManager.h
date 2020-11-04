#pragma once

#include "Lobby/PlayerEvents.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Containers/TArray.h"

class ChatManager
{
	friend class CrazyCanvas;

public:
	DECL_STATIC_CLASS(ChatManager);

	static void SendChatMessage(LambdaEngine::String message);

private:
	static void Init();
	static void Release();
	static LambdaEngine::NetworkSegment* MakePacket(LambdaEngine::IClient* pClient, uint64 uid, const LambdaEngine::String message);
	static bool OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event);
	static bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

private:
	static LambdaEngine::TArray<LambdaEngine::String> m_ChatHistory;
};