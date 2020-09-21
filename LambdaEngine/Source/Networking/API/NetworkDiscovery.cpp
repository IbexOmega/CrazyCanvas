#include "Networking/API/NetworkDiscovery.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/INetworkDiscoveryServer.h"
#include "Networking/API/INetworkDiscoveryClient.h"

namespace LambdaEngine
{
	NetworkDiscovery NetworkDiscovery::s_Instance;

	NetworkDiscovery::NetworkDiscovery() : 
		m_pServer(nullptr),
		m_Lock()
	{

	}

	bool NetworkDiscovery::EnableServer(uint16 portOfGameServer, INetworkDiscoveryServer* pHandler, uint16 portOfBroadcastServer)
	{
		std::scoped_lock<SpinLock> lock(s_Instance.m_Lock);
		if (!s_Instance.m_pServer)
		{
			s_Instance.m_pServer = DBG_NEW ServerNetworkDiscovery();
			return s_Instance.m_pServer->Start(IPEndPoint(IPAddress::ANY, portOfBroadcastServer), portOfGameServer, pHandler);
		}
		return false;
	}

	void NetworkDiscovery::DisableServer()
	{
		std::scoped_lock<SpinLock> lock(s_Instance.m_Lock);
		if (s_Instance.m_pServer)
		{
			s_Instance.m_pServer->Release();
			s_Instance.m_pServer = nullptr;
		}
	}

	bool NetworkDiscovery::IsServerEnabled()
	{
		return s_Instance.m_pServer;
	}

	bool NetworkDiscovery::EnableClient(INetworkDiscoveryClient* pHandler)
	{
		UNREFERENCED_VARIABLE(pHandler);
		return true;
	}

	void NetworkDiscovery::DisableClient()
	{
	}

	bool NetworkDiscovery::IsClientEnabled()
	{
		return false;
	}

	void NetworkDiscovery::FixedTick(Timestamp delta)
	{
		UNREFERENCED_VARIABLE(delta);
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

	void NetworkDiscovery::OnServerFull(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void NetworkDiscovery::ReleaseStatic()
	{
		DisableServer();
		DisableClient();
	}
}
