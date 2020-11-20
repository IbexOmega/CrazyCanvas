#include "Chat/ChatManager.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/ChatEvents.h"

#include "Lobby/PlayerManagerClient.h"

#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"

#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketType.h"


using namespace LambdaEngine;

TArray<ChatMessage> ChatManager::m_ChatHistory;

void ChatManager::Init()
{
	EventQueue::RegisterEventHandler<NetworkSegmentReceivedEvent>(&ChatManager::OnPacketReceived);
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(&ChatManager::OnClientConnected);
}

void ChatManager::Release()
{
	EventQueue::UnregisterEventHandler<NetworkSegmentReceivedEvent>(&ChatManager::OnPacketReceived);
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(&ChatManager::OnClientConnected);
}

NetworkSegment* ChatManager::MakePacket(IClient* pClient, const ChatMessage& message)
{
	NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CHAT_MESSAGE);
	BinaryEncoder encoder(pPacket);
	encoder.WriteBool(message.IsRecap);
	encoder.WriteUInt64(message.UID);
	encoder.WriteUInt8(message.Team);
	encoder.WriteString(message.Name);
	encoder.WriteString(message.Message);
	return pPacket;
}

void ChatManager::RenotifyAllChatMessages()
{
	for (ChatMessage& message : m_ChatHistory)
	{
		message.IsRecap = true;
		ChatEvent event(message);
		EventQueue::SendEventImmediate(event);
	}
}

void ChatManager::SendChatMessage(String message)
{
	if (MultiplayerUtils::IsServer())
	{
		ServerBase* pServer = ServerSystem::GetInstance().GetServer();
		if (pServer->GetClientCount() > 0)
		{
			ClientRemoteBase* pClientRemoteBase = pServer->GetClients().begin()->second;
			const ChatMessage& chatMessage = { false, UINT64_MAX, UINT8_MAX, "Server", message };
			NetworkSegment* pPacket = MakePacket(pClientRemoteBase, chatMessage);
			m_ChatHistory.PushBack(chatMessage);
			pClientRemoteBase->SendReliableBroadcast(pPacket, nullptr, true);

			ChatEvent event(chatMessage);
			EventQueue::SendEventImmediate(event);
		}
	}
	else
	{
		const Player* pPlayer = PlayerManagerClient::GetPlayerLocal();

		if (pPlayer)
		{
			LOG_INFO("[CHAT] %s: %s", pPlayer->GetName().c_str(), message.c_str());

			IClient* pClient = ClientSystem::GetInstance().GetClient();
			const ChatMessage& chatMessage = { false, pPlayer->GetUID(), pPlayer->GetTeam(), pPlayer->GetName(), message };
			NetworkSegment* pPacket = MakePacket(pClient, chatMessage);
			m_ChatHistory.PushBack(chatMessage);
			pClient->SendReliable(pPacket);

			ChatEvent event(chatMessage);
			EventQueue::SendEventImmediate(event);
		}
	}
}

bool ChatManager::OnPacketReceived(const NetworkSegmentReceivedEvent& event)
{
	if (event.Type == PacketType::CHAT_MESSAGE)
	{
		ChatMessage chatMessage;
		BinaryDecoder decoder(event.pPacket);
		decoder.ReadBool(chatMessage.IsRecap);
		decoder.ReadUInt64(chatMessage.UID);
		decoder.ReadUInt8(chatMessage.Team);
		decoder.ReadString(chatMessage.Name);
		decoder.ReadString(chatMessage.Message);

		if (chatMessage.UID == UINT64_MAX)
		{
			if (!MultiplayerUtils::IsServer())
			{
				m_ChatHistory.PushBack(chatMessage);

				ChatEvent newEvent(chatMessage);
				EventQueue::SendEventImmediate(newEvent);
			}
		}
		else
		{
			if (MultiplayerUtils::IsServer())
			{
				const Player* pPlayer = PlayerManagerBase::GetPlayer(event.pClient->GetUID());

				if (pPlayer)
				{
					chatMessage.IsRecap = false;
					chatMessage.UID = pPlayer->GetUID();
					chatMessage.Team = pPlayer->GetTeam();
					chatMessage.Name = pPlayer->GetName();

					if (MultiplayerUtils::IsServer())
					{
						ClientRemoteBase* pClientRemoteBase = (ClientRemoteBase*)event.pClient;
						NetworkSegment* pPacket = MakePacket(pClientRemoteBase, chatMessage);
						pClientRemoteBase->SendReliableBroadcast(pPacket, nullptr, true);
					}
				}
			}

			m_ChatHistory.PushBack(chatMessage);

			LOG_INFO("[CHAT] %s: %s", chatMessage.Name.c_str(), chatMessage.Message.c_str());

			ChatEvent newEvent(chatMessage);
			EventQueue::SendEventImmediate(newEvent);
		}
		
		return true;
	}
	return false;
}

bool ChatManager::OnClientConnected(const ClientConnectedEvent& event)
{
	if (MultiplayerUtils::IsServer())
	{
		IClient* pClient = event.pClient;

		for (ChatMessage& message : m_ChatHistory)
		{
			message.IsRecap = true;
			NetworkSegment* pPacket = MakePacket(pClient, message);
			pClient->SendReliable(pPacket);
		}
	}
	return false;
}