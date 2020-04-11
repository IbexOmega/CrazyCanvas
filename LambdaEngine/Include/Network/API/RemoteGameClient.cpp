#include "RemoteGameClient.h"

#include "GameServer.h"

#include "IGameClientHandler.h"

#include "NetworkPacket.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RemoteGameClient::RemoteGameClient(const std::string& address, uint16 port, uint64 hash, GameServer* pGameServer, IGameClientHandler* handler) :
		m_Address(address),
		m_Port(port),
		m_Hash(hash),
		m_pGameServer(pGameServer),
		m_pHandler(handler),
		m_PacketBufferIndex(0)
	{
		m_PacketBuffers[0] = new std::vector<NetworkPacket*>();
		m_PacketBuffers[1] = new std::vector<NetworkPacket*>();
	}

	RemoteGameClient::~RemoteGameClient()
	{
		delete m_PacketBuffers[0];
		delete m_PacketBuffers[1];
	}

	bool RemoteGameClient::AddPacket(NetworkPacket* packet)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketBuffers);
		m_PacketBuffers[m_PacketBufferIndex]->push_back(packet);
		return true;
	}

	const std::string& RemoteGameClient::GetAddress() const
	{
		return m_Address;
	}

	uint16 RemoteGameClient::GetPort() const
	{
		return m_Port;
	}

	uint64 RemoteGameClient::GetHash() const
	{
		return m_Hash;
	}

	std::vector<NetworkPacket*>* RemoteGameClient::SwapAndGetPacketBuffer()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketBuffers);
		std::vector<NetworkPacket*>* buffer = m_PacketBuffers[m_PacketBufferIndex];
		m_PacketBufferIndex = m_PacketBufferIndex % 2;
		return buffer;
	}

	bool RemoteGameClient::TransmitPackets(char* buffer, int32 size)
	{
		std::vector<NetworkPacket*>* packets = SwapAndGetPacketBuffer();

		while (!packets->empty())
		{
			PACKET_SIZE size = sizeof(PACKET_SIZE) + sizeof(uint8);
			uint8 nrOfPackets = 0;
			while (true)
			{
				NetworkPacket* packet = RemovePacketOfSizeLOE(packets, MAXIMUM_PACKET_SIZE - size);
				if (packet)
				{
					nrOfPackets++;
					packet->Pack();
					PACKET_SIZE packetSize = packet->GetSize();
					memcpy(buffer + size, packet->GetBuffer(), packetSize);
					size += packetSize;
				}
				else
				{
					memcpy(buffer, &size, sizeof(PACKET_SIZE));
					memcpy(buffer + sizeof(PACKET_SIZE), &nrOfPackets, sizeof(uint8));

					if (!m_pGameServer->Transmit(GetAddress(), GetPort(), buffer, size))
						return false;

					break;
				}
			}
		}
		return true;
	}

	void RemoteGameClient::OnPacketReceived(char* buffer, int32 bytesReceived)
	{
		PACKET_SIZE totalPacketSize = 0;
		memcpy(&totalPacketSize, buffer, sizeof(PACKET_SIZE));

		if (bytesReceived == totalPacketSize)
		{
			uint8 nrOfPackets = 0;
			memcpy(&nrOfPackets, buffer, sizeof(uint8));

			if (nrOfPackets > 0)
			{
				ExtractPackets(buffer + sizeof(PACKET_SIZE) + sizeof(uint8), nrOfPackets);
			}
			else
			{
				LOG_ERROR("[RemoteGameClient]: No packets contained in received buffer");
			}
		}
		else
		{
			LOG_ERROR("[RemoteGameClient]: Received buffer has a size missmatch");
		}
	}

	void RemoteGameClient::ExtractPackets(char* buffer, uint8 packets)
	{
		NetworkPacket packet(PACKET_TYPE_UNDEFINED, false);
		int32 offset = 0;
		for (int i = 0; i < packets; i++)
		{
			memcpy(packet.GetBuffer(), buffer + offset, sizeof(PACKET_SIZE));
			packet.UnPack();
			memcpy(packet.GetBuffer(), buffer + offset, packet.GetSize());
			offset += packet.GetSize();

			m_pHandler->OnPacketReceived(this, &packet);
		}
	}

	NetworkPacket* RemoteGameClient::RemovePacketOfSizeLOE(std::vector<NetworkPacket*>* packets, PACKET_SIZE size)
	{
		for (int i = 0; i < packets->size(); i++)
		{
			NetworkPacket* packet = packets->at(i);
			if (packet->GetSize() <= size)
			{
				packets->insert(packets->begin() + i, packet);
				packets->pop_back();
				return packet;
			}
		}
		return nullptr;
	}
}