#include "Network/API/Discovery/NetworkDiscoverySearcher.h"
#include "Network/API/PlatformNetworkUtils.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	std::set<NetworkDiscoverySearcher*> NetworkDiscoverySearcher::s_Searchers;
	uint64 NetworkDiscoverySearcher::s_Timer;
	SpinLock NetworkDiscoverySearcher::m_Lock;

	NetworkDiscoverySearcher::NetworkDiscoverySearcher(INetworkDiscoverySearcherHandler* pHandler, const std::string& uid) : 
		m_pHandler(pHandler), 
		m_UID(uid),
		m_pClient(nullptr),
		m_Packet(PACKET_TYPE_NETWORK_DISCOVERY, false)
	{
		m_pClient = PlatformNetworkUtils::CreateClientUDP(this);
		m_pClient->Start(ADDRESS_BROADCAST, 4450);

		std::scoped_lock<SpinLock> lock(m_Lock);
		s_Searchers.insert(this);
	}

	NetworkDiscoverySearcher::~NetworkDiscoverySearcher()
	{
		m_pClient->Release();

		std::scoped_lock<SpinLock> lock(m_Lock);
		s_Searchers.erase(this);
	}

	void NetworkDiscoverySearcher::BroadcastPacket()
	{
		m_Packet.Reset();
		m_Packet.WriteString(m_UID);
		m_pClient->SendPacket(&m_Packet, true);
	}

	void NetworkDiscoverySearcher::OnClientPacketReceivedUDP(IClientUDP* client, NetworkPacket* packet)
	{
		PACKET_TYPE type = packet->ReadPacketType();
		if (type == PACKET_TYPE_NETWORK_DISCOVERY)
		{
			std::string address = packet->ReadString();
			uint16 port = packet->ReadUInt16();
			m_pHandler->OnHostFound(address, port, packet);
		}
		UNREFERENCED_VARIABLE(client);
	}

	void NetworkDiscoverySearcher::OnClientErrorUDP(IClientUDP* client)
	{
		UNREFERENCED_VARIABLE(client);
	}

	void NetworkDiscoverySearcher::OnClientStoppedUDP(IClientUDP* client)
	{
		UNREFERENCED_VARIABLE(client);
	}

	void NetworkDiscoverySearcher::InitStatic()
	{
		s_Timer = 0;
	}

	void NetworkDiscoverySearcher::TickStatic(Timestamp dt)
	{
		constexpr uint64 PING_DELAY = 1000000000;

		s_Timer += dt.AsNanoSeconds();
		if (s_Timer >= PING_DELAY)
		{
			s_Timer -= PING_DELAY;

			std::scoped_lock<SpinLock> lock(m_Lock);
			for (NetworkDiscoverySearcher* searcher : s_Searchers)
			{
				searcher->BroadcastPacket();
			}
		}
	}
}
