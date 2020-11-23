#pragma once

#include "Events/PlayerEvents.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Containers/TArray.h"

struct ChatMessage
{
	bool IsRecap;
	uint64 UID;
	uint8 Team;
	LambdaEngine::String Name;
	LambdaEngine::String Message;
};

class ChatManager
{
	friend class CrazyCanvas;

public:
	DECL_STATIC_CLASS(ChatManager);

	static void SendChatMessage(LambdaEngine::String message);
	static void RenotifyAllChatMessages();

private:
	static void Init();
	static void Release();
	static LambdaEngine::NetworkSegment* MakePacket(LambdaEngine::IClient* pClient, const ChatMessage& message);
	static bool OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event);
	static bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

private:
	static LambdaEngine::TArray<ChatMessage> m_ChatHistory;
};