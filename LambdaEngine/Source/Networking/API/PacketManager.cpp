#include "Networking/API/PacketManager.h"

#include "Networking/API/NetworkPacket.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiver.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManager::PacketManager() :
		m_PacketPool(1024),
		m_QueueIndex(0)
	{

	}

	PacketManager::~PacketManager()
	{

	}

	uint32 PacketManager::EnqueuePacketReliable(NetworkPacket* pPacket, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);
		uint32 UID = EnqueuePacket(pPacket, m_Statistics.RegisterReliableMessageSent());
		m_MessagesWaitingForAck.insert({ UID, MessageInfo{ pPacket, pListener, EngineLoop::GetTimeSinceStart()} });
		return UID;
	}

	uint32 PacketManager::EnqueuePacketUnreliable(NetworkPacket* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);
		return EnqueuePacket(pPacket, 0);
	}

	uint32 PacketManager::EnqueuePacket(NetworkPacket* pPacket, uint32 reliableUID)
	{
		pPacket->GetHeader().UID = m_Statistics.RegisterMessageSent();
		pPacket->GetHeader().ReliableUID = reliableUID;
		m_MessagesToSend[m_QueueIndex].push(pPacket);
		return pPacket->GetHeader().UID;
	}

	void PacketManager::Flush(PacketTransceiver* pTransceiver)
	{
		int32 indexToUse = m_QueueIndex;
		m_QueueIndex = (m_QueueIndex + 1) % 2;
		std::queue<NetworkPacket*>& packets = m_MessagesToSend[indexToUse];

		Timestamp timestamp = EngineLoop::GetTimeSinceStart();

		while (!packets.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_PacketPool, packets, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
				bundle.Timestamp = timestamp;

				std::scoped_lock<SpinLock> lock(m_LockBundles);
				m_Bundles.insert({ bundleUID, bundle });
			}
		}
	}

	void PacketManager::QueryBegin(PacketTransceiver* pTransceiver, std::vector<NetworkPacket*>& packetsReturned)
	{
		std::vector<NetworkPacket*> packets;
		IPEndPoint ipEndPoint;
		std::vector<uint32> acks;

		if (!pTransceiver->ReceiveEnd(&m_PacketPool, packets, acks, &m_Statistics))
			return;

		packetsReturned.clear();
		packetsReturned.reserve(packets.size());

		HandleAcks(acks);
		FindPacketsToReturn(packets, packetsReturned);
	}

	void PacketManager::QueryEnd(std::vector<NetworkPacket*>& packetsReceived)
	{
		m_PacketPool.FreePackets(packetsReceived);
	}

	void PacketManager::Tick(Timestamp delta)
	{
		static const Timestamp delay = Timestamp::Seconds(1);

		m_Timer += delta;
		if (m_Timer >= delay)
		{
			m_Timer -= delay;
			DeleteOldBundles();
		}

		ResendOrDeleteMessages();
	}

	PacketPool* PacketManager::GetPacketPool()
	{
		return &m_PacketPool;
	}

	const NetworkStatistics* PacketManager::GetStatistics() const
	{
		return &m_Statistics;
	}

	const IPEndPoint& PacketManager::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void PacketManager::SetEndPoint(const IPEndPoint& ipEndPoint)
	{
		m_IPEndPoint = ipEndPoint;
	}

	void PacketManager::Reset()
	{
		std::scoped_lock<SpinLock> lock1(m_LockMessagesToSend);
		std::scoped_lock<SpinLock> lock2(m_LockBundles);
		m_MessagesToSend[0] = {};
		m_MessagesToSend[1] = {};
		m_MessagesWaitingForAck.clear();
		m_ReliableMessagesReceived.clear();
		m_Bundles.clear();

		m_PacketPool.Reset();
		m_Statistics.Reset();
		m_QueueIndex = 0;
	}

	void PacketManager::FindPacketsToReturn(const std::vector<NetworkPacket*>& packetsReceived, std::vector<NetworkPacket*>& packetsReturned)
	{
		bool reliableMessagesInserted = false;
		bool hasReliableMessage = false;

		std::vector<NetworkPacket*> packetsToFree;
		packetsToFree.reserve(32);

		for (NetworkPacket* pPacket : packetsReceived)
		{
			if (!pPacket->IsReliable())																//Unreliable Packet
			{
				if (pPacket->GetType() == NetworkPacket::TYPE_NETWORK_ACK)
					packetsToFree.push_back(pPacket);
				else
					packetsReturned.push_back(pPacket);
			}
			else
			{
				hasReliableMessage = true;

				if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)		//Reliable Packet in correct order
				{
					packetsReturned.push_back(pPacket);
					m_Statistics.RegisterReliableMessageReceived();
				}
				else if (pPacket->GetReliableUID() > m_Statistics.GetLastReceivedReliableUID())		//Reliable Packet in incorrect order
				{
					m_ReliableMessagesReceived.insert(pPacket);
					reliableMessagesInserted = true;
				}
				else																				//Reliable Packet already received before
				{
					packetsToFree.push_back(pPacket);
				}
			}
		}

		m_PacketPool.FreePackets(packetsToFree);

		if (reliableMessagesInserted)
			UntangleReliablePackets(packetsReturned);

		if (hasReliableMessage && m_MessagesToSend[m_QueueIndex].empty())
			EnqueuePacketUnreliable(m_PacketPool.RequestFreePacket()->SetType(NetworkPacket::TYPE_NETWORK_ACK));
	}

	void PacketManager::UntangleReliablePackets(std::vector<NetworkPacket*>& packetsReturned)
	{
		std::vector<NetworkPacket*> packetsToErase;

		for (NetworkPacket* pPacket : m_ReliableMessagesReceived)
		{
			if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)
			{
				packetsReturned.push_back(pPacket);
				packetsToErase.push_back(pPacket);
				m_Statistics.RegisterReliableMessageReceived();
			}
			else
			{
				break;
			}
		}

		for (NetworkPacket* pPacket : packetsToErase)
		{
			m_ReliableMessagesReceived.erase(pPacket);
		}
	}

	void PacketManager::HandleAcks(const std::vector<uint32>& acks)
	{
		std::vector<uint32> ackedReliableUIDs;
		GetReliableUIDsFromAcks(acks, ackedReliableUIDs);

		std::vector<MessageInfo> messagesAcked;
		GetReliableMessageInfosFromUIDs(ackedReliableUIDs, messagesAcked);

		std::vector<NetworkPacket*> packetsToFree;
		packetsToFree.reserve(messagesAcked.size());

		for (MessageInfo& messageInfo : messagesAcked)
		{
			if (messageInfo.Listener)
			{
				messageInfo.Listener->OnPacketDelivered(messageInfo.Packet);
			}
			packetsToFree.push_back(messageInfo.Packet);
		}
	}

	void PacketManager::GetReliableUIDsFromAcks(const std::vector<uint32>& acks, std::vector<uint32>& ackedReliableUIDs)
	{
		ackedReliableUIDs.reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockBundles);

		Timestamp timestamp = 0;

		for (uint32 ack : acks)
		{
			auto& iterator = m_Bundles.find(ack);
			if (iterator != m_Bundles.end())
			{
				Bundle& bundle = iterator->second;
				for (uint32 UID : bundle.ReliableUIDs)
					ackedReliableUIDs.push_back(UID);

				timestamp = bundle.Timestamp;
				m_Bundles.erase(iterator);
			}
		}

		if (timestamp != 0)
		{
			RegisterRTT(EngineLoop::GetTimeSinceStart() - timestamp);
		}
	}

	void PacketManager::GetReliableMessageInfosFromUIDs(const std::vector<uint32>& ackedReliableUIDs, std::vector<MessageInfo>& ackedReliableMessages)
	{
		ackedReliableMessages.reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);

		for (uint32 UID : ackedReliableUIDs)
		{
			auto& iterator = m_MessagesWaitingForAck.find(UID);
			if (iterator != m_MessagesWaitingForAck.end())
			{
				ackedReliableMessages.push_back(iterator->second);
				m_MessagesWaitingForAck.erase(iterator);
			}
		}
	}

	void PacketManager::RegisterRTT(Timestamp rtt)
	{
		static const double scalar1 = 1.0f / 10.0f;
		static const double scalar2 = 1.0f - scalar1;
		m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
	}

	void PacketManager::DeleteOldBundles()
	{
		Timestamp maxAllowedTime = m_Statistics.GetPing() * 100;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		std::vector<uint32> bundlesToDelete;

		std::scoped_lock<SpinLock> lock(m_LockBundles);
		for (auto& pair : m_Bundles)
		{
			if (currentTime - pair.second.Timestamp > maxAllowedTime)
			{
				bundlesToDelete.push_back(pair.first);
				m_Statistics.RegisterPacketLoss();
			}
		}

		for (uint32 UID : bundlesToDelete)
			m_Bundles.erase(UID);
	}

	void PacketManager::ResendOrDeleteMessages()
	{
		Timestamp maxAllowedTime = m_Statistics.GetPing() * 2;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		std::vector<std::pair<const uint32, MessageInfo>> messagesToDelete;

		{
			std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);

			for (auto& pair : m_MessagesWaitingForAck)
			{
				MessageInfo& messageInfo = pair.second;
				if (currentTime - messageInfo.LastSent > maxAllowedTime)
				{
					messageInfo.Retries++;

					if (messageInfo.Retries < 10)
						m_MessagesToSend[m_QueueIndex].push(messageInfo.Packet);
					else
						messagesToDelete.push_back(pair);
				}
			}

			for (auto& pair : messagesToDelete)
				m_MessagesWaitingForAck.erase(pair.first);
		}
		
		for (auto& pair : messagesToDelete)
		{
			MessageInfo& messageInfo = pair.second;
			if (messageInfo.Listener)
				messageInfo.Listener->OnPacketMaxTriesReached(messageInfo.Packet, messageInfo.Retries);
		}
	}
}