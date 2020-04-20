#include "Engine/EngineLoop.h"
#include "Log/Log.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketManager.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	PacketManager::PacketManager(uint16 packets, uint8 maximumTries) :
		m_NrOfPackets(packets),
		m_QueueIndex(0),
		m_PacketsCounter(0),
		m_MessageCounter(0),
		m_ReceivedSequenceBits(0),
		m_LastReceivedSequenceNr(0),
		m_Salt(0),
		m_SaltRemote(0),
		m_Ping(UINT64_MAX),
		m_MaximumTries(maximumTries)
	{
		m_pPackets = DBG_NEW NetworkPacket[packets];
		Reset();
	}

	PacketManager::~PacketManager()
	{
		delete[] m_pPackets;
	}

	void PacketManager::EnqueuePacket(NetworkPacket* packet)
	{
		EnqueuePacket(packet, nullptr);
	}

	void PacketManager::EnqueuePacket(NetworkPacket* packet, IPacketListener* listener)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		packet->GetHeader().UID = GetNextMessageUID();
		m_PacketsToSend[m_QueueIndex].push(MessageInfo{ packet, listener});
	}

	NetworkPacket* PacketManager::GetFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		if(!m_PacketsFree.empty())
		{
			NetworkPacket* packet = *m_PacketsFree.rbegin();
			m_PacketsFree.erase(packet);
			packet->m_SizeOfBuffer = 0;
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
				if (messageInfo.Listener)
				{
					bundle.MessageUIDs.insert(messageInfo.GetUID());
					reliableMessages.push_back(messageInfo);
				}

				header.Packets++;

				//Have we Processed all messages or have we reach the limit of 32
				if (packets.empty())
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
					return true;
				}
				else if (header.Packets == 32)
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
					return false;
				}
			}
			else
			{
				WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle, reliableMessages);
				return false;
			}
		}
		return true;
	}

	void PacketManager::WriteHeaderAndStoreBundle(char* buffer, int32& bytesWritten, Header& header, Bundle& bundle, std::vector<MessageInfo>& reliableMessages)
	{
		header.Sequence = GetNextPacketSequenceNr();
		header.Salt		= GetSalt();
		header.Ack		= m_LastReceivedSequenceNr;
		header.AckBits	= m_ReceivedSequenceBits;
		memcpy(buffer, &header, sizeof(Header));

		bytesWritten = header.Size;

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

	bool PacketManager::DecodePackets(const char* buffer, int32 bytesReceived, NetworkPacket** packetsRead, int32& nrOfPackets)
	{
		Header header;
		uint16 offset = sizeof(Header);

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
		else if (m_SaltRemote != header.Salt)
		{
			if (m_SaltRemote == 0)
			{
				m_SaltRemote = header.Salt;
			}
			else
			{
				LOG_ERROR("[PacketManager]: Received a packet with a new salt [Prev %lu : New %lu]", m_SaltRemote.load(), header.Salt);
				return false;
			}
		}

		ProcessSequence(header.Sequence);
		ProcessAcks(header.Ack, header.AckBits);

		for (int i = 0; i < header.Packets; i++)
		{
			NetworkPacket* message = GetFreePacket();
			NetworkPacket::Header& messageHeader = message->GetHeader();
			uint8 messageHeaderSize = message->GetHeaderSize();

			memcpy(&messageHeader, buffer + offset, messageHeaderSize);
			memcpy(message->GetBuffer(), buffer + offset + messageHeaderSize, messageHeader.Size - messageHeaderSize);
			offset += messageHeader.Size;
			message->m_Salt = header.Salt;
			packetsRead[i] = message;
		}
		nrOfPackets = header.Packets;
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

	void PacketManager::Free(NetworkPacket** packets, int32 nrOfPackets)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		for (int i = 0; i < nrOfPackets; i++)
		{
			NetworkPacket* packet = packets[i];
			if (packet)
			{
				m_PacketsFree.insert(packet);
			}
		}
	}

	const Timestamp& PacketManager::GetPing() const
	{
		return m_Ping;
	}

	uint64 PacketManager::GetSalt() const
	{
		return m_Salt;
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

		m_Salt = Random::UInt64();
		m_SaltRemote = 0;
		m_Ping = Timestamp::MilliSeconds(10.0f);
	}

	void PacketManager::ProcessSequence(uint32 sequence)
	{
		if (sequence > m_LastReceivedSequenceNr)
		{
			//New sequence number received so shift everything delta steps
			int32 delta = sequence - m_LastReceivedSequenceNr;
			m_ReceivedSequenceBits = m_ReceivedSequenceBits << delta;
			m_LastReceivedSequenceNr = sequence;
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
			m_Ping = (rtt.AsNanoSeconds() * scalar1) + (m_Ping.AsNanoSeconds() * scalar2);
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

			for (MessageInfo& message : messages)
			{
				if (message.Listener)
				{
					message.Listener->OnPacketDelivered(message.Packet);
				}
			}
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
		return ++m_PacketsCounter;
	}

	uint32 PacketManager::GetNextMessageUID()
	{
		return ++m_MessageCounter;
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
		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);

		for (auto& pair : m_MessagesWaitingForAck)
		{
			delta = EngineLoop::GetTimeSinceStart() - pair.second.LastSent;
			if (delta > GetPing() * 2.0F)
			{
				if (pair.second.Tries > m_MaximumTries)
				{
					packetsReachMaxTries.push_back(pair.second);
				}
				else
				{
					messages.push_back(pair.second);
				}
			}
		}

		for (MessageInfo& message : packetsReachMaxTries)
		{
			message.Listener->OnPacketMaxTriesReached(message.Packet, message.Tries);
			m_MessagesWaitingForAck.erase(message.GetUID());
		}
	}

	void PacketManager::ReSendMessages(const std::vector<MessageInfo>& messages)
	{
		for (MessageInfo messageInfo : messages)
		{
			messageInfo.Listener->OnPacketResent(messageInfo.Packet, messageInfo.Tries);
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