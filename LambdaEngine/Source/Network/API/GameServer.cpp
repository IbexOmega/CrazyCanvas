#include "Network/API/GameServer.h"
#include "Network/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	GameServer::GameServer(const std::string& name, uint8 maxClients) : 
		m_pServerTCP(nullptr),
		m_pServerUDP(nullptr),
		m_pServerNDH(this, name)
	{
		m_pServerTCP = PlatformNetworkUtils::CreateServerTCP(this, maxClients);
		m_pServerUDP = PlatformNetworkUtils::CreateServerUDP(this);
	}

	GameServer::~GameServer()
	{
		m_pServerTCP->Release();
		m_pServerUDP->Release();
	}

	bool GameServer::Start()
	{
		m_AddressBound = PlatformNetworkUtils::GetLocalAddress();
		uint16 port = 4444; //NOT A GOOD WAY

		if (!m_pServerTCP->Start(m_AddressBound, port))
			return false;

		if (!m_pServerUDP->Start(m_AddressBound, port))
			return false;

		if (!m_pServerNDH.Start(m_AddressBound, port))
			return false;

		return true;
	}

	IClientTCPHandler* GameServer::CreateClientHandlerTCP()
	{
		return nullptr;
	}

	bool GameServer::OnClientAcceptedTCP(ClientTCP* client)
	{
		return false;
	}

	void GameServer::OnClientConnectedTCP(ClientTCP* client)
	{

	}

	void GameServer::OnClientDisconnectedTCP(ClientTCP* client)
	{

	}

	IClientUDPHandler* GameServer::CreateClientHandlerUDP()
	{
		return nullptr;
	}

	void GameServer::OnSearcherRequest(NetworkPacket* packet)
	{

	}
}