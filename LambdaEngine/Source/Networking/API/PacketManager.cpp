#include "Engine/EngineLoop.h"
#include "Log/Log.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	PacketManager::PacketManager(uint16 packets) :
		m_QueueIndex(0),
		m_PacketsCounter(0),
		m_MessageCounter(0),
		m_ReceivedSequenceBits(0),
		m_LastReceivedSequenceNr(0)
	{
		m_pPackets = DBG_NEW NetworkPacket[packets];

		for (int i = 0; i < packets; i++)
		{
			m_PacketsFree.insert(&m_pPackets[i]);
		}
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
		m_PacketsToSend[m_QueueIndex].push(MessageInfo{ packet, listener, GetNextMessageUID() });
	}

	NetworkPacket* PacketManager::GetFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		if(!m_PacketsFree.empty())
		{
			NetworkPacket* packet = *m_PacketsFree.rbegin();
			m_PacketsFree.erase(packet);
			return packet;
		}
		LOG_ERROR("[PacketManager]: No free packets exist. Please increase the nr of packets");
		return nullptr;
	}

	bool PacketManager::EncodePackets(char* buffer, int32& bytesWritten)
	{
		int index = m_QueueIndex;
		m_QueueIndex = m_QueueIndex % 2;
		std::queue<MessageInfo>& packets = m_PacketsToSend[index];

		Header header;
		header.Size = sizeof(Header);

		NetworkPacket* packet = nullptr;
		uint16 packetBufferSize = 0;
		uint16 packetHeaderSize = 0;

		Bundle bundle;

		bytesWritten = 0;

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
					messageInfo.LastSent = EngineLoop::GetTimeSinceStart();
					bundle.Infos[header.Packets] = messageInfo;
				}
				else
				{
					bundle.Infos[header.Packets] = MessageInfo();
				}

				header.Packets++;

				//Have we Processed all messages or have we reach the limit of 32
				if (packets.empty())
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle);
					return true;
				}
				else if (header.Packets == 32)
				{
					WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle);
					return false;
				}
			}
			else
			{
				WriteHeaderAndStoreBundle(buffer, bytesWritten, header, bundle);
				return false;
			}
		}
		return true;
	}

	void PacketManager::WriteHeaderAndStoreBundle(char* buffer, int32& bytesWritten, Header& header, Bundle& bundle)
	{
		header.Sequence = GetNextPacketSequenceNr();
		header.Ack = m_LastReceivedSequenceNr;
		header.AckBits = m_ReceivedSequenceBits;
		memcpy(buffer, &header, sizeof(Header));

		bundle.Count = header.Packets;

		bytesWritten = header.Size;

		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
		m_PacketsWaitingForAck.insert({ header.Sequence, bundle });
	}

	bool PacketManager::DecodePackets(const char* buffer, int32 bytesReceived, NetworkPacket** packetsRead, int32& nrOfPackets)
	{
		Header header;
		uint16 offset = sizeof(Header);

		memcpy(&header, buffer, offset);

		if (header.Size != bytesReceived)
		{
			LOG_WARNING("[PacketManager]: Received a packet with size missmatch [Exp %d : Rec %d]", header.Size, bytesReceived);
			return false;
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
			packetsRead[i] = message;
		}
		nrOfPackets = header.Packets;
		return true;
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
		uint32 top = std::min(32, (int)ack);
		for (uint8 i = 1; i <= top; i++)
		{
			if (ackBits & 1)
			{
				ProcessAck(ack - i);
			}
			ackBits >>= 1;
		}
		ProcessAck(ack);
	}

	void PacketManager::ProcessAck(uint32 ack)
	{
		Bundle bundle;
		MessageInfo* messageInfo = nullptr;
		if (GetAndRemoveBundle(ack, bundle))
		{
			for (uint8 i = 0; i < bundle.Count; i++)
			{
				messageInfo = &bundle.Infos[i];
				if (messageInfo->Listener)
				{
					messageInfo->Listener->OnPacketDelivered(messageInfo->Packet);
				}
			}
		}
	}

	bool PacketManager::GetAndRemoveBundle(uint32 sequence, Bundle& bundle)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
		auto iterator = m_PacketsWaitingForAck.find(sequence);
		if (iterator != m_PacketsWaitingForAck.end())
		{
			bundle = iterator->second;
			m_PacketsWaitingForAck.erase(sequence);
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

	void PacketManager::Clear()
	{
		std::scoped_lock<SpinLock> lock1(m_LockPacketsFree);
		m_PacketsFree.clear();

		std::scoped_lock<SpinLock> lock2(m_LockPacketsToSend);
		m_PacketsToSend[0] = {};
		m_PacketsToSend[1] = {};

		std::scoped_lock<SpinLock> lock3(m_LockPacketsWaitingForAck);
		m_PacketsWaitingForAck.clear();
	}
}