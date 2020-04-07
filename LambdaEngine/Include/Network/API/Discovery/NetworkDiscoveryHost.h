#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>

#include "Network/API/UDP/IServerUDPHandler.h"
#include "Network/API/UDP/IClientUDPHandler.h"

#include "Network/API/UDP/ServerUDP.h"
#include "INetworkDiscoveryHostHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API NetworkDiscoveryHost : protected IServerUDPHandler, protected IClientUDPHandler
	{
	public:
		NetworkDiscoveryHost(INetworkDiscoveryHostHandler* pHandler, const std::string& uid, const std::string& address, uint16 port);
		~NetworkDiscoveryHost();
		
	protected:
		virtual IClientUDPHandler* CreateClientHandlerUDP() override;

		virtual void OnClientPacketReceivedUDP(IClientUDP* client, NetworkPacket* packet) override;
		virtual void OnClientErrorUDP(IClientUDP* client) override;
		virtual void OnClientStoppedUDP(IClientUDP* client) override;

	private:
		ServerUDP* m_pServer;
		INetworkDiscoveryHostHandler* m_pHandler;
		std::string m_UID;
		std::string m_Address;
		uint16 m_Port;
		NetworkPacket m_PacketResponse;
	};
}
