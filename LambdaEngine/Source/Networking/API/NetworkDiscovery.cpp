#include "Networking/API/NetworkDiscovery.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/INetworkDiscoveryHandler.h"

namespace LambdaEngine
{
	NetworkDiscovery NetworkDiscovery::s_Instance;

	NetworkDiscovery::NetworkDiscovery() : 
		m_pServer(nullptr),
		m_Lock()
	{

	}

	bool NetworkDiscovery::EnableDiscovery(INetworkDiscoveryHandler* pHandler)
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

			s_Instance.m_pHandler = pHandler;
			s_Instance.m_pServer = static_cast<ServerUDP*>(NetworkUtils::CreateServer(desc));
			return s_Instance.m_pServer->Start(IPEndPoint(IPAddress::ANY, 4450));
		}
		return true;
	}

	void NetworkDiscovery::FixedTick(Timestamp delta)
	{
		UNREFERENCED_VARIABLE(delta);
		static Timestamp interval = Timestamp::Seconds(1);

		if (EngineLoop::GetTimeSinceStart() - m_TimestampOfLastTransmit >= interval)
		{
			m_TimestampOfLastTransmit = EngineLoop::GetTimeSinceStart();
			const ClientMap& clients = s_Instance.m_pServer->GetClients();
			if (!clients.empty())
			{
				ClientRemoteBase* pClient = clients.begin()->second;
				NetworkSegment* pSegment = pClient->GetFreePacket(NetworkSegment::TYPE_NETWORK_DISCOVERY);
				s_Instance.m_pHandler->OnNetworkDiscoveryPreTransmit(pSegment);
				pClient->SendReliableBroadcast(pSegment);
			}
		}
	}

	IClientRemoteHandler* NetworkDiscovery::CreateClientHandler()
	{
		return this;
	}

	void NetworkDiscovery::OnConnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void NetworkDiscovery::OnConnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void NetworkDiscovery::OnDisconnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void NetworkDiscovery::OnDisconnected(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void NetworkDiscovery::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		UNREFERENCED_VARIABLE(pClient);
		UNREFERENCED_VARIABLE(pPacket);
	}

	void NetworkDiscovery::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}
}
