#include "PacketDispatcher.h"
#include "ISocketUDP.h"

namespace LambdaEngine
{
	PacketDispatcher::PacketDispatcher() : 
		m_QueueIndex(0)
	{

	}

	PacketDispatcher::~PacketDispatcher()
	{

	}

	void PacketDispatcher::EnqueuePacket(NetworkPacket* packet)
	{
		EnqueuePacket(packet, nullptr);
	}

	void PacketDispatcher::EnqueuePacket(NetworkPacket* packet, IPacketListener* listener)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
		m_PacketsToSend[m_QueueIndex].push(NetworkPair{ packet, listener });
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

		PacketDispatcherHeader dispatcher;
		dispatcher.Size = sizeof(PacketDispatcherHeader);

		while (!packets.empty())
		{
			NetworkPair& pair = packets.front();
			NetworkPacket* packet = pair.Packet;
			uint16 packetBufferSize = packet->GetBufferSize();
			uint16 packetHeaderSize = packet->GetHeaderSize();

			if (packet->GetTotalSize() + dispatcher.Size < MAXIMUM_PACKET_SIZE)
			{
				packets.pop();
				memcpy(m_pSendBuffer + dispatcher.Size, &packet->GetHeader(), packetHeaderSize);
				dispatcher.Size += packetHeaderSize;
				memcpy(m_pSendBuffer + dispatcher.Size, packet->GetBufferReadOnly(), packetBufferSize);
				dispatcher.Size += packetBufferSize;
				dispatcher.Packets++;

				if (pair.Listener)
				{
					//pair.LastSent = 
					m_PacketsWaitingForAck.insert({ packet->GetHeader().UID, pair });
				}
			}
			else
			{
				//dispatcher.UID = ;
				memcpy(m_pSendBuffer, &dispatcher, sizeof(PacketDispatcherHeader));
				int32 bytesSent = 0;

				if (!socket->SendTo(m_pSendBuffer, dispatcher.Size, bytesSent, destination))
				{
					Clear();
					return false;
				}

				dispatcher.Packets = 0;
				dispatcher.Size = sizeof(PacketDispatcherHeader);
			}
		}

		return true;
	}

	void PacketDispatcher::Clear()
	{

	}
}