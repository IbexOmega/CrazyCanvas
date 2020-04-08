#include "NetworkDiscoveryHost.h"

#include "Network/API/UDP/IClientUDP.h"
#include "Network/API/ISocket.h"
#include "Network/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	NetworkDiscoveryHost::NetworkDiscoveryHost(INetworkDiscoveryHostHandler* pHandler, const std::string& uid, const std::string& address, uint16 port) : 
		m_pHandler(pHandler),
		m_pServer(nullptr),
		m_UID(uid),
		m_Address(address),
		m_Port(port),
		m_PacketResponse(PACKET_TYPE_NETWORK_DISCOVERY, false)
	{
		m_pServer = PlatformNetworkUtils::CreateServerUDP(this);
		m_pServer->Start(ADDRESS_ANY, 4450);
	}

	NetworkDiscoveryHost::~NetworkDiscoveryHost()
	{
		m_pServer->Release();
	}

	IClientUDPHandler* NetworkDiscoveryHost::CreateClientHandlerUDP()
	{
		return this;
	}

	void NetworkDiscoveryHost::OnClientPacketReceivedUDP(IClientUDP* client, NetworkPacket* packet)
	{
		PACKET_TYPE type = packet->ReadPacketType();
		if (type == PACKET_TYPE_NETWORK_DISCOVERY)
		{
			if (m_UID == packet->ReadString())
			{
				m_PacketResponse.Reset();
				m_PacketResponse.WriteString(m_Address);
				m_PacketResponse.WriteUInt16(m_Port);
				m_pHandler->OnSearcherRequest(&m_PacketResponse);
				client->SendPacketImmediately(&m_PacketResponse);
			}
		}
		client->Release();
	}

	void NetworkDiscoveryHost::OnClientErrorUDP(IClientUDP* client)
	{
		UNREFERENCED_VARIABLE(client);
	}

	void NetworkDiscoveryHost::OnClientStoppedUDP(IClientUDP* client)
	{
		UNREFERENCED_VARIABLE(client);
	}
}
