#include "Engine/EngineLoop.h"
#include "Log/Log.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketDispatcher.h"
#include "Networking/API/IDispatcherHandler.h"

namespace LambdaEngine
{
	PacketDispatcher::PacketDispatcher(uint16 packets, IDispatcherHandler* pHandler) :
		m_QueueIndex(0),
		m_PacketsCounter(0),
		m_MessageCounter(0),
		m_pSendBuffer(),
		m_pReceiveBuffer(),
		m_ReceivedHistory(),
		m_pHandler(pHandler),
		m_LastReceivedSequenceNr(0)
	{
		m_pPackets = new NetworkPacket[packets];
		m_PacketsFree.resize(packets);

		for (int i = 0; i < packets; i++)
		{
			m_PacketsFree.push_back(&m_pPackets[i]);
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
		m_PacketsToSend[m_QueueIndex].push(NetworkPair{ packet, listener, GetNextMessageUID() });
	}

	NetworkPacket* PacketDispatcher::GetFreePacket()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsFree);
		if(!m_PacketsFree.empty())
		{
			NetworkPacket* packet = m_PacketsFree[m_PacketsFree.size() - 1];
			m_PacketsFree.pop_back();
			return packet;
		}
		return nullptr;
	}

	bool PacketDispatcher::Dispatch(ISocketUDP* socket, const IPEndPoint& destination)
	{
		int index = m_QueueIndex;
		m_QueueIndex = m_QueueIndex % 2;
		std::queue<NetworkPair>& packets = m_PacketsToSend[index];

		PacketDispatcherHeader header;
		header.Size = sizeof(PacketDispatcherHeader);

		NetworkPacket* packet = nullptr;
		uint16 packetBufferSize = 0;
		uint16 packetHeaderSize = 0;

		BundledPacket bundle;

		while (!packets.empty())
		{
			NetworkPair& pair = packets.front();
			packet = pair.Packet;
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

				if (pair.Listener)
				{
					pair.LastSent = EngineLoop::GetTimeSinceStart();
					bundle.NetworkPairs[header.Packets] = pair;
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
	}

	bool PacketDispatcher::Send(ISocketUDP* socket, const IPEndPoint& destination, PacketDispatcherHeader& header, BundledPacket& bundle)
	{
		header.Sequence = GetNextPacketSequenceNr();
		header.Ack = m_LastReceivedSequenceNr;
		memcpy(m_pSendBuffer, &header, sizeof(PacketDispatcherHeader));

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
		header.Size = sizeof(PacketDispatcherHeader);
		return true;
	}

	bool PacketDispatcher::Receive(ISocketUDP* socket)
	{
		int32 bytesReceived = 0;
		IPEndPoint sender(IPAddress::NONE, 0);
		PacketDispatcherHeader header;
		NetworkPacket* packet = GetFreePacket();
		NetworkPacketHeader& messageHeader = packet->GetHeader();
		uint8 messageHeaderSize = sizeof(NetworkPacketHeader);
		NetworkPair pair;
		uint16 offset = 0;

		while (true)
		{
			offset = sizeof(PacketDispatcherHeader);
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

						m_pHandler->OnPacketReceived(packet);
					}
				}
				else
				{
					LOG_WARNING("[PacketDispatcher]: Received a packet with size missmatch [Exp %d : Rec %d]", header.Size, bytesReceived);
				}
			}
		}

		return false;
	}

	void PacketDispatcher::ProcessSequence(uint32 sequence)
	{
		if (sequence > m_LastReceivedSequenceNr)
		{
			int32 delta = sequence - m_LastReceivedSequenceNr;
			for (int8 i = 31; i >= 0; i--)
				m_ReceivedHistory[i] = m_ReceivedHistory[i - delta];

			for (uint8 i = 1; i < delta; i++)
				m_ReceivedHistory[i] = false;

			m_ReceivedHistory[0] = true;
			m_LastReceivedSequenceNr = sequence;
		}
		else if (m_LastReceivedSequenceNr - sequence < 32)
		{
			m_ReceivedHistory[m_LastReceivedSequenceNr - sequence];
		}
	}

	void PacketDispatcher::ProcessAcks(uint32 ack, uint32 ackBits)
	{
		BundledPacket bundle;
		NetworkPair* pair = nullptr;
		uint32 top = std::min(32, (int)ack);

		for (uint8 i = 0; i < top; i++)
		{
			if (ackBits & 1)
			{
				if (GetAndRemoveBundle(ack - i, bundle))
				{
					for (uint8 j = 0; j < bundle.Count; j++)
					{
						pair = &bundle.NetworkPairs[j];
						pair->Listener->OnPacketDelivered(pair->Packet);
					}
				}
			}

			ackBits >>= 1;
		}
	}

	bool PacketDispatcher::GetAndRemoveBundle(uint32 sequence, BundledPacket& bundle)
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