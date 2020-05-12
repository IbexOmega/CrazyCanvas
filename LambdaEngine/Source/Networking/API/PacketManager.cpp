#include "Networking/API/PacketManager.h"

#include "Networking/API/NetworkPacket.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiver.h"

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
		m_MessagesWaitingForAck.insert({ UID, MessageInfo{ pPacket, pListener } });
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

		while (!packets.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_PacketPool, packets, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
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

	void PacketManager::Tick()
	{

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

		for (NetworkPacket* pPacket : packetsReceived)
		{
			if (!pPacket->IsReliable())																//Unreliable Packet
			{
				packetsReturned.push_back(pPacket);
			}
			else if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)	//Reliable Packet in correct order
			{
				packetsReturned.push_back(pPacket);
				m_Statistics.RegisterReliableMessageReceived();
			}
			else if(pPacket->GetReliableUID() > m_Statistics.GetLastReceivedReliableUID())			//Reliable Packet in incorrect order
			{
				m_ReliableMessagesReceived.insert(pPacket);
				reliableMessagesInserted = true;
			}
			else																					//Reliable Packet already received before
			{
				m_PacketPool.FreePacket(pPacket);
			}
		}

		if(reliableMessagesInserted)
			UntangleReliablePackets(packetsReturned);
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

		for (uint32 ack : acks)
		{
			auto iterator = m_Bundles.find(ack);
			if (iterator != m_Bundles.end())
			{
				Bundle& bundle = iterator->second;
				for (uint32 UID : bundle.ReliableUIDs)
					ackedReliableUIDs.push_back(UID);

				m_Bundles.erase(iterator);
			}
		}
	}

	void PacketManager::GetReliableMessageInfosFromUIDs(const std::vector<uint32>& ackedReliableUIDs, std::vector<MessageInfo>& ackedReliableMessages)
	{
		ackedReliableMessages.reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockMessagesToSend);

		for (uint32 UID : ackedReliableUIDs)
		{
			auto iterator = m_MessagesWaitingForAck.find(UID);
			if (iterator != m_MessagesWaitingForAck.end())
			{
				ackedReliableMessages.push_back(iterator->second);
				m_MessagesWaitingForAck.erase(iterator);
			}
		}
	}
}
