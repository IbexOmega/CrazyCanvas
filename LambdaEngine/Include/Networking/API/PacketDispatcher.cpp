#include "Engine/EngineLoop.h"
#include "Log/Log.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketDispatcher.h"

namespace LambdaEngine
{
	PacketDispatcher::PacketDispatcher(uint16 packets) :
		m_QueueIndex(0),
		m_PacketsCounter(0),
		m_MessageCounter(0),
		m_ReceivedSequenceBits(0),
		m_LastReceivedSequenceNr(0)
	{
		m_pPackets = new NetworkPacket[packets];

		for (int i = 0; i < packets; i++)
		{
			m_PacketsFree.insert(&m_pPackets[i]);
		}
	}

	PacketDispatcher::~PacketDispatcher()
	{
		delete[] m_pPackets;
	}

	void PacketDispatcher::EnqueuePacket(NetworkPacket* packet)
	{
		EnqueuePacket(packet, nullptr);
	}

	void PacketDispatcher::EnqueuePacket(NetworkPacket* packet, IPacketListener* listener)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		m_PacketsToSend[m_QueueIndex].push(MessageInfo{ packet, listener, GetNextMessageUID() });
	}

	NetworkPacket* PacketDispatcher::GetFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		if(!m_PacketsFree.empty())
		{
			NetworkPacket* packet = *m_PacketsFree.rbegin();
			m_PacketsFree.erase(packet);
			return packet;
		}
		return nullptr;
	}

	bool PacketDispatcher::EncodePackets(char* buffer, int32& bytesWritten)
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

	/*bool PacketDispatcher::EncodePackets(ISocketUDP* socket, const IPEndPoint& destination)
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

		while (!packets.empty())
		{
			MessageInfo& messageInfo = packets.front();
			packet = messageInfo.Packet;
			packetBufferSize = packet->GetBufferSize();
			packetHeaderSize = packet->GetHeaderSize();

			if (packet->GetTotalSize() + header.Size < MAXIMUM_PACKET_SIZE)
			{
				packets.pop();
				packet->GetHeader().Size = packet->GetTotalSize();
				memcpy(m_pSendBuffer + header.Size, &packet->GetHeader(), packetHeaderSize);
				header.Size += packetHeaderSize;
				memcpy(m_pSendBuffer + header.Size, packet->GetBufferReadOnly(), packetBufferSize);
				header.Size += packetBufferSize;

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

				if (header.Packets == 32 || packets.empty())
				{
					if (!Send(socket, destination, header, bundle))
						return false;
				}
			}
			else
			{
				if (!Send(socket, destination, header, bundle))
					return false;
			}
		}

		return true;
	}*/

	/*bool PacketDispatcher::Send(ISocketUDP* socket, const IPEndPoint& destination, Header& header, Bundle& bundle)
	{
		header.Sequence = GetNextPacketSequenceNr();
		header.Ack = m_LastReceivedSequenceNr;
		header.AckBits = m_ReceivedSequenceBits;
		memcpy(m_pSendBuffer, &header, sizeof(Header));

		bundle.Count = header.Packets;

		{
			std::scoped_lock<SpinLock> lock(m_LockPacketsWaitingForAck);
			m_PacketsWaitingForAck.insert({ header.Sequence, bundle });
		}

		int32 bytesSent = 0;

		if (!socket->SendTo(m_pSendBuffer, header.Size, bytesSent, destination))
		{
			Clear();
			return false;
		}

		header.Packets = 0;
		header.Size = sizeof(Header);
		return true;
	}*/

	void PacketDispatcher::WriteHeaderAndStoreBundle(char* buffer, int32& bytesWritten, Header& header, Bundle& bundle)
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

	bool PacketDispatcher::DecodePackets(const char* buffer, int32 bytesReceived, NetworkPacket** packetsRead, int32& nrOfPackets)
	{
		Header header;
		uint16 offset = sizeof(Header);

		memcpy(&header, buffer, offset);

		if (header.Size != bytesReceived)
		{
			LOG_WARNING("[PacketDispatcher]: Received a packet with size missmatch [Exp %d : Rec %d]", header.Size, bytesReceived);
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

	void PacketDispatcher::Free(NetworkPacket** packets, int32 nrOfPackets)
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

	/*bool PacketDispatcher::Receive(ISocketUDP* socket, const char* buffer, NetworkPacket* packetsRead[32])
	{
		int32 bytesReceived = 0;
		IPEndPoint sender(IPAddress::NONE, 0);
		Header header;
		NetworkPacket* packet = GetFreePacket();
		NetworkPacket::Header& messageHeader = packet->GetHeader();
		uint8 messageHeaderSize = packet->GetHeaderSize();
		uint16 offset = 0;

		while (true)
		{
			offset = sizeof(Header);
			if (socket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX_, bytesReceived, sender))
			{
				memcpy(&header, m_pReceiveBuffer, offset);
				if (header.Size == bytesReceived)
				{
					ProcessSequence(header.Sequence);
					ProcessAcks(header.Ack, header.AckBits);

					for (int i = 0; i < header.Packets; i++)
					{
						memcpy(&messageHeader, m_pReceiveBuffer + offset, messageHeaderSize);
						memcpy(packet->GetBuffer(), m_pReceiveBuffer + offset + messageHeaderSize, messageHeader.Size - messageHeaderSize);
						offset += messageHeader.Size;

						m_pHandler->OnPacketReceived(packet, sender);
					}
				}
				else
				{
					LOG_WARNING("[PacketDispatcher]: Received a packet with size missmatch [Exp %d : Rec %d]", header.Size, bytesReceived);
				}
			}
			else
			{
				break;
			}
		}

		return false;
	}*/

	void PacketDispatcher::ProcessSequence(uint32 sequence)
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

	void PacketDispatcher::ProcessAcks(uint32 ack, uint32 ackBits)
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

	void PacketDispatcher::ProcessAck(uint32 ack)
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

	bool PacketDispatcher::GetAndRemoveBundle(uint32 sequence, Bundle& bundle)
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

	uint32 PacketDispatcher::GetNextPacketSequenceNr()
	{
		return ++m_PacketsCounter;
	}

	uint32 PacketDispatcher::GetNextMessageUID()
	{
		return ++m_MessageCounter;
	}

	void PacketDispatcher::Clear()
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