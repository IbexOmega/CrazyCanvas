#include "Networking/API/NetworkDiscovery.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"

namespace LambdaEngine
{
	NetworkDiscovery NetworkDiscovery::s_Instance;

	NetworkDiscovery::NetworkDiscovery() : 
		m_pServer(nullptr),
		m_Lock()
	{

	}

	void NetworkDiscovery::FixedTick(Timestamp delta)
	{

	}

	bool NetworkDiscovery::EnableDiscovery()
	{
		std::scoped_lock<SpinLock> lock(s_Instance.m_Lock);
		if (!s_Instance.m_pServer)
		{
			ServerDesc desc = {};
			desc.Handler		= &s_Instance;
			desc.MaxRetries		= 0;
			desc.MaxClients		= 100;
			desc.PoolSize		= 128;
			desc.Protocol		= EProtocol::UDP;
			desc.PingInterval	= Timestamp::Seconds(60);
			desc.PingTimeout	= Timestamp::Seconds(3);
			desc.UsePingSystem	= true;

			s_Instance.m_pServer = static_cast<ServerUDP*>(NetworkUtils::CreateServer(desc));
			return s_Instance.m_pServer->Start(IPEndPoint(IPAddress::ANY, 4450));
		}
		return true;
	}

	IClientRemoteHandler* NetworkDiscovery::CreateClientHandler()
	{
		return this;
	}

	void NetworkDiscovery::OnConnecting(IClient* pClient)
	{

	}

	void NetworkDiscovery::OnConnected(IClient* pClient)
	{
		
	}

	void NetworkDiscovery::OnDisconnecting(IClient* pClient)
	{

	}

	void NetworkDiscovery::OnDisconnected(IClient* pClient)
	{

	}

	void NetworkDiscovery::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{

	}

	void NetworkDiscovery::OnClientReleased(IClient* pClient)
	{

	}
}