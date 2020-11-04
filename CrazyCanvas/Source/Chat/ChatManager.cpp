#include "Chat/ChatManager.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerManager.h"

#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"

#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketType.h"

using namespace LambdaEngine;

TArray<String> ChatManager::m_ChatHistory;

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

NetworkSegment* ChatManager::MakePacket(LambdaEngine::IClient* pClient, uint64 uid, const LambdaEngine::String message)
{
	NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CHAT_MESSAGE);
	BinaryEncoder encoder(pPacket);
	encoder.WriteUInt64(uid);
	encoder.WriteString(message);
	return pPacket;
}

void ChatManager::SendChatMessage(String message)
{
	if (MultiplayerUtils::IsServer())
	{
		m_ChatHistory.PushBack("[Server] " + message);

		ServerBase* pServer = ServerSystem::GetInstance().GetServer();
		if (pServer->GetClientCount() > 0)
		{
			SystemChatEvent newEvent(message);
			EventQueue::SendEventImmediate(newEvent);

			ClientRemoteBase* pClientRemoteBase = pServer->GetClients().begin()->second;
			NetworkSegment* pPacket = MakePacket(pClientRemoteBase, UINT64_MAX, message);
			pClientRemoteBase->SendReliableBroadcast(pPacket, nullptr, true);
		}
	}
	else
	{
		const Player* pPlayer = PlayerManager::GetPlayerLocal();

		if (pPlayer)
		{
			m_ChatHistory.PushBack("[" + pPlayer->GetName() + "] " + message);

			LOG_INFO("[CHAT] %s: %s", pPlayer->GetName().c_str(), message.c_str());

			PlayerChatEvent newEvent(*pPlayer, message);
			EventQueue::SendEventImmediate(newEvent);

			IClient* pClient = ClientSystem::GetInstance().GetClient();
			NetworkSegment* pPacket = MakePacket(pClient, pPlayer->GetUID(), message);
			pClient->SendReliable(pPacket);
		}
	}
}

bool ChatManager::OnPacketReceived(const NetworkSegmentReceivedEvent& event)
{
	if (event.Type == PacketType::CHAT_MESSAGE)
	{
		BinaryDecoder decoder(event.pPacket);
		uint64 playerUID = decoder.ReadUInt64();
		String message = decoder.ReadString();

		if (playerUID == UINT64_MAX)
		{
			if (!MultiplayerUtils::IsServer())
			{
				m_ChatHistory.PushBack("[Server] " + message);

				SystemChatEvent newEvent(message);
				EventQueue::SendEventImmediate(newEvent);
			}
		}
		else if (playerUID == 0)
		{
			if (!MultiplayerUtils::IsServer())
			{
				m_ChatHistory.PushBack(message);

				LOG_INFO("[CHAT_RECAP] %s", message.c_str());

				PlayerChatRecapEvent newEvent(message);
				EventQueue::SendEventImmediate(newEvent);
			}
		}
		else
		{
			const Player* pPlayer = PlayerManager::GetPlayer(playerUID);

			if (pPlayer)
			{
				m_ChatHistory.PushBack("[" + pPlayer->GetName() + "] " + message);

				LOG_INFO("[CHAT] %s: %s", pPlayer->GetName().c_str(), message.c_str());

				PlayerChatEvent newEvent(*pPlayer, message);
				EventQueue::SendEventImmediate(newEvent);

				if (MultiplayerUtils::IsServer())
				{
					ClientRemoteBase* pClientRemoteBase = (ClientRemoteBase*)event.pClient;
					NetworkSegment* pPacket = MakePacket(pClientRemoteBase, pClientRemoteBase->GetUID(), message);
					pClientRemoteBase->SendReliableBroadcast(pPacket, nullptr, true);
				}
			}
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

		for (const String& message : m_ChatHistory)
		{
			NetworkSegment* pPacket = MakePacket(pClient, 0, message);
			pClient->SendReliable(pPacket);
		}
	}
	return false;
}