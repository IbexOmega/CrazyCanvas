#pragma once
#include "Containers/String.h"

#include "Defines.h"
#include "Types.h"

#include "TCP/IServerTCPHandler.h"
#include "UDP/IServerUDPHandler.h"

#include "Network/API/Discovery/NetworkDiscoveryHost.h"

namespace LambdaEngine
{
	class ServerTCP;
	class ServerUDP;

	class LAMBDA_API GameServer : 
		protected IServerTCPHandler,
		protected IServerUDPHandler,
		protected INetworkDiscoveryHostHandler
	{
	public:
		GameServer(const std::string& name, uint8 maxClients);
		~GameServer();

		bool Start();

	protected:
		virtual IClientTCPHandler* CreateClientHandlerTCP() override;
		virtual bool OnClientAcceptedTCP(ClientTCP* client) override;
		virtual void OnClientConnectedTCP(ClientTCP* client) override;
		virtual void OnClientDisconnectedTCP(ClientTCP* client) override;

		virtual IClientUDPHandler* CreateClientHandlerUDP() override;

		virtual void OnSearcherRequest(NetworkPacket* packet) override;

	private:
		ServerTCP* m_pServerTCP;
		ServerUDP* m_pServerUDP;
		NetworkDiscoveryHost m_pServerNDH;
		std::string m_AddressBound;
	};
}
