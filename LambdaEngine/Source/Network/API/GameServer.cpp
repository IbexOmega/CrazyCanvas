#include "Network/API/GameServer.h"
#include "Network/API/PlatformNetworkUtils.h"
#include "Network/API/RemoteGameClient.h"

namespace LambdaEngine
{
	GameServer::GameServer()
	{
		
	}

	GameServer::~GameServer()
	{
		
	}

	void GameServer::RunTranmitter()
	{
		while (!ShouldTerminate())
		{

			//TransmitPackets(SwapAndGetPacketBuffer());
		}
	}

	void GameServer::RunReceiver()
	{
		int32 bytesReceived = 0;
		std::string address;
		uint16 port;

		while (!ShouldTerminate())
		{
			if (m_pSocket->ReceiveFrom(m_ReceiveBuffer, MAXIMUM_DATAGRAM_SIZE, bytesReceived, address, port))
			{
				RemoteGameClient* client = GetOrCreateClient(address, port);
				client->OnPacketReceived(m_ReceiveBuffer, bytesReceived);









				/*memcpy(packet->GetBuffer(), m_ReceiveBuffer, sizeof(PACKET_SIZE));
				packet->Reset();
				packet->UnPack();
				if (bytesReceived == packet->GetSize())
				{
					memcpy(packet->GetBuffer(), m_ReceiveBuffer, bytesReceived);
					uint64 hash = Hash(address, port);
					ClientUDPRemote* client = nullptr;
					{
						std::scoped_lock<SpinLock> lock(m_Lock);
						client = m_Clients[hash];
					}
					if (!client)
					{
						IClientUDPHandler* handler = m_pHandler->CreateClientHandlerUDP();
						client = DBG_NEW ClientUDPRemote(address, port, hash, this, handler);
						std::scoped_lock<SpinLock> lock(m_Lock);
						m_Clients[hash] = client;
					}
					client->OnPacketReceived(packet);
				}
				else
				{
					LOG_ERROR("Error received a packet with the wrong size %i | %i", bytesReceived, packet->GetSize());
				}
				RegisterPacketsReceived(1);*/
			}
			else
			{
				Stop();
			}
		}
	}

	void GameServer::OnThreadsTurminated()
	{

	}

	RemoteGameClient* GameServer::GetOrCreateClient(const std::string& address, uint64 port)
	{
		uint64 hash = Hash(address, port);

		std::unordered_map<int64, RemoteGameClient*>::iterator iterator;
		{
			std::scoped_lock<SpinLock> lock(m_LockClients);
			iterator = m_Clients.find(hash);
		}

		if (iterator != m_Clients.end())
			return iterator->second;

		RemoteGameClient* client = DBG_NEW RemoteGameClient(address, port, hash, this, nullptr);/// Should not be nullptr
		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.insert({ hash, client });
		return client;
	}

	uint64 GameServer::Hash(const std::string& address, uint64 port)
	{
		std::hash<std::string> hasher;
		return port ^ hasher(address) + 0x9e3779b9 + (port << 6) + (port >> 2);
	}
}