#include "Engine/EngineLoop.h"
#include "Log/Log.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketManager.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	PacketManager::PacketManager(IPacketListener* pListener, uint16 packets, uint8 maximumTries) :
		m_pListener(pListener),
		m_NrOfPackets(packets),
		m_MaximumTries(maximumTries)
	{
		m_pPackets = DBG_NEW NetworkPacket[packets];
		Reset();
	}

	PacketManager::~PacketManager()
	{
		delete[] m_pPackets;
	}

	void PacketManager::EnqueuePacket(NetworkPacket* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		pPacket->GetHeader().UID = GetNextMessageUID();
		pPacket->GetHeader().ReliableUID = 0;
		m_PacketsToSend[m_QueueIndex].push(MessageInfo{ pPacket, nullptr });
	}

	void PacketManager::EnqueuePacketReliable(NetworkPacket* pPacket, IPacketListener* listener)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		pPacket->GetHeader().UID = GetNextMessageUID();
		pPacket->GetHeader().ReliableUID = GetNextMessageReliableUID();
		m_PacketsToSend[m_QueueIndex].push(MessageInfo{ pPacket, listener});
	}

	NetworkPacket* PacketManager::GetFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		if(!m_PacketsFree.empty())
		{
			NetworkPacket* packet = *m_PacketsFree.rbegin();
			m_PacketsFree.erase(packet);
			packet->m_SizeOfBuffer = 0;
			packet->GetHeader().ReliableUID = 0;
			return packet;
		}
		LOG_ERROR("[PacketManager]: No free packets exist. Please increase the nr of packets");
		return nullptr;
	}

	bool PacketManager::EncodePackets(char* buffer, int32& bytesWritten)
	{
		std::queue<MessageInfo>& packets = m_PacketsToSend[(m_QueueIndex + 1) % 2];

		Header header;
		header.Size = sizeof(Header);

		NetworkPacket* packet = nullptr;
		uint16 packetBufferSize = 0;
		uint16 packetHeaderSize = 0;

		Bundle bundle;

		bytesWritten = 0;

		std::vector<MessageInfo> reliableMessages;
		std::vector<NetworkPacket*> nonReliableMessages;

		while (!packets.empty())
		{
			MessageInfo& messageInfo = packets.front();
			packet = messageInfo.Packet;
			packetBufferSize = packet->GetBufferSize();
			packetHeaderSize = packet->GetHeaderSize();

			//Makes sure there is enough space to add the message
			if (packet->GetTotalSize() + header.Size < MAXIMUM_PACKET_SIZE)
			{
				packets.pop();
				packet->GetHeader().Size = packet->GetTotalSize();
				memcpy(buffer + header.Size, &packet->GetHeader(), packetHeaderSize);
				header.Size += packetHeaderSize;
				memcpy(buffer + header.Size, packet->GetBufferReadOnly(), packetBufferSize);
				header.Size += packetBufferSize;

				//Is this a reliable message ?
				if (messageInfo.Packet->IsReliable())
				{
					bundle.MessageUIDs.insert(messageInfo.GetUID());
					reliableMessages.push_back(messageInfo);
				}
				else
				{
					nonReliableMessages.push_back(messageInfo.Packet);
				}

				header.Packets++;

				//Have we Processed all messages or have we reach the limit of 32
				if (packets.empty())
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
					Free(nonReliableMessages);
					return true;
				}
				else if (header.Packets == 32)
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
					Free(nonReliableMessages);
					return false;
				}
			}
			else
			{
				WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
				Free(nonReliableMessages);
				return false;
			}
		}
		return true;
	}

	void PacketManager::WriteHeaderAndStoreBundle(char* buffer, int32& bytesWritten, Header& header, Bundle& bundle, std::vector<MessageInfo>& reliableMessages)
	{
		header.Sequence = GetNextPacketSequenceNr();
		header.Salt		= m_Statistics.GetSalt();
		header.Ack		= m_LastReceivedSequenceNr;
		header.AckBits	= m_ReceivedSequenceBits;
		memcpy(buffer, &header, sizeof(Header));

		bytesWritten = header.Size;
		m_Statistics.m_BytesSent += bytesWritten;

		bundle.Timestamp = EngineLoop::GetTimeSinceStart();

		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
		m_PacketsWaitingForAck.insert({ header.Sequence, bundle });
		for (MessageInfo& message : reliableMessages)
		{
			message.LastSent = bundle.Timestamp;
			message.Tries++;
			m_MessagesWaitingForAck.insert_or_assign(message.GetUID(), message);
		}
	}

	bool PacketManager::DecodePackets(const char* buffer, int32 bytesReceived, std::vector<NetworkPacket*>& packetsRead)
	{
		Header header;
		uint16 offset = sizeof(Header);
		m_Statistics.m_BytesReceived += bytesReceived;
		memcpy(&header, buffer, offset);

		if (header.Size != bytesReceived)
		{
			LOG_ERROR("[PacketManager]: Received a packet with size missmatch [Exp %d : Rec %d]", header.Size, bytesReceived);
			return false;
		}
		else if (header.Salt == 0)
		{
			LOG_ERROR("[PacketManager]: Received a packet without a salt");
			return false;
		}
		else if (m_Statistics.GetRemoteSalt() != header.Salt)
		{
			if (m_Statistics.GetRemoteSalt() == 0)
			{
				m_Statistics.m_SaltRemote = header.Salt;
			}
			else
			{
				LOG_ERROR("[PacketManager]: Received a packet with a new salt [Prev %lu : New %lu]", m_Statistics.GetRemoteSalt(), header.Salt);
				return false;
			}
		}

		ProcessSequence(header.Sequence);
		ProcessAcks(header.Ack, header.AckBits);

		packetsRead.reserve(header.Packets);

		std::vector<NetworkPacket*> messagesToFree;
		bool hasReliableMessage = false;

		for (int i = 0; i < header.Packets; i++)
		{
			NetworkPacket* message = GetFreePacket();
			NetworkPacket::Header& messageHeader = message->GetHeader();
			uint8 messageHeaderSize = message->GetHeaderSize();

			memcpy(&messageHeader, buffer + offset, messageHeaderSize);
			memcpy(message->GetBuffer(), buffer + offset + messageHeaderSize, messageHeader.Size - messageHeaderSize);
			offset += messageHeader.Size;
			message->m_Salt = header.Salt;

			if (message->GetType() == NetworkPacket::TYPE_NETWORK_ACK)
			{
				messagesToFree.push_back(message);
			}
			else if (message->IsReliable())
			{
				hasReliableMessage = true;
				if (message->GetReliableUID() >= m_NextExpectedMessageNr)
				{
					m_MessagesReceivedOrdered.insert(message);
				}
				else
				{
					messagesToFree.push_back(message);
				}
			}
			else
			{
				packetsRead.push_back(message);
			}
		}

		Free(messagesToFree);

		std::vector<NetworkPacket*> messagesComplete;
		for (NetworkPacket* message : m_MessagesReceivedOrdered)
		{
			if (message->GetReliableUID() != m_NextExpectedMessageNr)
			{
				LOG_MESSAGE("Skipped %d != %d", message->GetReliableUID(), m_NextExpectedMessageNr);
				break;
			}

			m_NextExpectedMessageNr++;
			packetsRead.push_back(message);
			messagesComplete.push_back(message);
		}

		for (NetworkPacket* message : messagesComplete)
		{
			m_MessagesReceivedOrdered.erase(message);
		}

		if (hasReliableMessage && m_PacketsToSend[m_QueueIndex].empty())
		{
			NetworkPacket* pPacket = GetFreePacket();
			pPacket->SetType(NetworkPacket::TYPE_NETWORK_ACK);
			EnqueuePacket(pPacket);
		}

		return true;
	}

	void PacketManager::SwapPacketQueues()
	{
		m_QueueIndex = (m_QueueIndex + 1) % 2;
	}

	void PacketManager::Tick()
	{
		if (m_PacketsWaitingForAck.empty() && m_MessagesWaitingForAck.empty())
			return;

		std::vector<MessageInfo> m_MessagesToResend;
		FindMessagesToResend(m_MessagesToResend);
		ReSendMessages(m_MessagesToResend);
		DeleteEmptyBundles();
	}

	void PacketManager::Free(std::vector<NetworkPacket*>& packets)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		for (NetworkPacket* pPacket : packets)
		{
			m_PacketsFree.insert(pPacket);
		}
		packets.clear();
	}

	const NetworkStatistics* PacketManager::GetStatistics() const
	{
		return &m_Statistics;
	}

	void PacketManager::Reset()
	{
		{
			std::scoped_lock<SpinLock> lock1(m_LockPacketsFree);
			m_PacketsFree.clear();
			for (int i = 0; i < m_NrOfPackets; i++)
			{
				m_PacketsFree.insert(&m_pPackets[i]);
			}
		}
		
		{
			std::scoped_lock<SpinLock> lock2(m_LockPacketsToSend);
			m_PacketsToSend[0] = {};
			m_PacketsToSend[1] = {};
		}

		{
			std::scoped_lock<SpinLock> lock3(m_LockPacketsWaitingForAck);
			m_PacketsWaitingForAck.clear();
			m_MessagesWaitingForAck.clear();
		}

		m_QueueIndex = 0;
		m_ReceivedSequenceBits = 0;
		m_LastReceivedSequenceNr = 0;
		m_NextExpectedMessageNr = 1;
		m_ReliableMessagesSent = 1;

		m_Statistics.Reset();
	}

	void PacketManager::ProcessSequence(uint32 sequence)
	{
		if (sequence > m_LastReceivedSequenceNr)
		{
			//New sequence number received so shift everything delta steps
			int32 delta = sequence - m_LastReceivedSequenceNr;
			m_LastReceivedSequenceNr = sequence;

			for (int i = 0; i < delta; i++)
			{
				if (m_ReceivedSequenceBits >> (sizeof(uint32) * 8 - 1) & 0)
				{
					//The last bit that gets removed was never acked.
					m_Statistics.m_PacketsLost++;
				}
				m_ReceivedSequenceBits = m_ReceivedSequenceBits << 1;
			}
		}
		else
		{
			//Old sequence number received so write 1 to the coresponding bit
			int32 index = m_LastReceivedSequenceNr - sequence - 1;
			if (index >= 0 && index < 32)
			{
				int32 mask = 1 << index;
				m_ReceivedSequenceBits |= mask;
			}
		}
	}

	void PacketManager::ProcessAcks(uint32 ack, uint32 ackBits)
	{
		Timestamp rtt = UINT64_MAX;
		uint32 top = std::min(32, (int32)ack);

		for (uint8 i = 1; i <= top; i++)
		{
			if (ackBits & 1)
			{
				ProcessAck(ack - i, rtt);
			}
			ackBits >>= 1;
		}
		ProcessAck(ack, rtt);

		static const double scalar1 = 1.0 / 10.0;
		static const double scalar2 = 1.0 - scalar1;
		if (rtt != UINT64_MAX)
		{
			m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
		}
	}

	void PacketManager::ProcessAck(uint32 ack, Timestamp& rtt)
	{
		std::vector<MessageInfo> messages;
		Timestamp timestamp;
		if (GetMessagesAndRemoveBundle(ack, messages, timestamp))
		{
			timestamp = EngineLoop::GetTimeSinceStart() - timestamp;
			if (timestamp < rtt)
			{
				rtt = timestamp;
			}

			std::vector<NetworkPacket*> messagesToFree;
			messagesToFree.reserve(messages.size());
			for (MessageInfo& message : messages)
			{
				messagesToFree.push_back(message.Packet);

				if (message.Listener)
				{
					message.Listener->OnPacketDelivered(message.Packet);
				}
				m_pListener->OnPacketDelivered(message.Packet);
			}

			Free(messagesToFree);
		}
	}

	bool PacketManager::GetMessagesAndRemoveBundle(uint32 sequence, std::vector<MessageInfo>& messages, Timestamp& sentTimestamp)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
		auto packetIterator = m_PacketsWaitingForAck.find(sequence);
		if (packetIterator != m_PacketsWaitingForAck.end())
		{
			sentTimestamp = packetIterator->second.Timestamp;
			for (uint32 messageUID : packetIterator->second.MessageUIDs)
			{
				auto messageIterator = m_MessagesWaitingForAck.find(messageUID);
				if (messageIterator != m_MessagesWaitingForAck.end())
				{
					messages.push_back(messageIterator->second);
					m_MessagesWaitingForAck.erase(messageIterator);
				}
			}
			m_PacketsWaitingForAck.erase(packetIterator);
			return true;
		}
		return false;
	}

	uint32 PacketManager::GetNextPacketSequenceNr()
	{
		return m_Statistics.m_PacketsSent++;
	}

	uint32 PacketManager::GetNextMessageUID()
	{
		return m_Statistics.m_MessagesSent++;
	}

	uint32 PacketManager::GetNextMessageReliableUID()
	{
		return m_ReliableMessagesSent++;
	}

	void PacketManager::DeleteEmptyBundles()
	{
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();
		Timestamp maxTimeToStoreBundle = Timestamp::Seconds(1);
		std::vector<uint32> bundlesToRemove;
		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
		for (auto& pair : m_PacketsWaitingForAck)
		{
			std::vector<uint32> UIDsToRemove;
			std::set<uint32>& messageUIDs = pair.second.MessageUIDs;

			for (uint32 uid : messageUIDs)
			{
				auto messageIterator = m_MessagesWaitingForAck.find(uid);
				if (messageIterator == m_MessagesWaitingForAck.end())
				{
					UIDsToRemove.push_back(uid);
				}
			}

			for (uint32 uid : UIDsToRemove)
			{
				messageUIDs.erase(uid);
			}

			if (messageUIDs.empty() && currentTime - pair.second.Timestamp >= maxTimeToStoreBundle)
			{
				bundlesToRemove.push_back(pair.first);
			}
		}

		for (uint32 uid : bundlesToRemove)
		{
			m_PacketsWaitingForAck.erase(uid);
		}
	}

	void PacketManager::FindMessagesToResend(std::vector<MessageInfo>& messages)
	{
		Timestamp delta;
		std::vector<MessageInfo> packetsReachMaxTries;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		{
			std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);

			for (auto& pair : m_MessagesWaitingForAck)
			{
				delta = EngineLoop::GetTimeSinceStart() - pair.second.LastSent;
				if ((float64)delta.AsNanoSeconds() > (float64)m_Statistics.GetPing().AsNanoSeconds() * 2.0F)
				{
					if (pair.second.Tries > m_MaximumTries)
					{
						packetsReachMaxTries.push_back(pair.second);
					}
					else
					{
						pair.second.LastSent = currentTime;
						messages.push_back(pair.second);
					}
				}
			}

			for (MessageInfo& message : packetsReachMaxTries)
			{
				m_MessagesWaitingForAck.erase(message.GetUID());
			}
		}
		
		for (MessageInfo& messageInfo : packetsReachMaxTries)
		{
			if(messageInfo.Listener)
				messageInfo.Listener->OnPacketMaxTriesReached(messageInfo.Packet, messageInfo.Tries);

			m_pListener->OnPacketMaxTriesReached(messageInfo.Packet, messageInfo.Tries);
		}
	}

	void PacketManager::ReSendMessages(const std::vector<MessageInfo>& messages)
	{
		for (MessageInfo messageInfo : messages)
		{
			if (messageInfo.Listener)
				messageInfo.Listener->OnPacketResent(messageInfo.Packet, messageInfo.Tries);

			m_pListener->OnPacketResent(messageInfo.Packet, messageInfo.Tries);
		}

		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		for (MessageInfo messageInfo : messages)
		{
			m_PacketsToSend[m_QueueIndex].push(messageInfo);
		}
	}

	uint64 PacketManager::DoChallenge(uint64 clientSalt, uint64 serverSalt)
	{
		return clientSalt & serverSalt;
	}
}