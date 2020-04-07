#include "Network/API/NetworkUtils.h"

#include "Network/API/TCP/ClientTCP.h"

#include "Network/API/UDP/ClientUDP.h"
#include "Network/API/UDP/ISocketUDP.h"

#include "Network/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	bool NetworkUtils::Init()
	{
		ClientTCP::InitStatic();
		return true;
	}

	void NetworkUtils::Tick(Timestamp dt)
	{
		ClientTCP::TickStatic(dt);
	}

	void NetworkUtils::Release()
	{
		ClientTCP::ReleaseStatic();
	}

	IClientTCP* NetworkUtils::CreateClientTCP(IClientTCPHandler* handler)
	{
		return DBG_NEW ClientTCP(handler);
	}

	IClientUDP* NetworkUtils::CreateClientUDP(IClientUDPHandler* handler)
	{
		return DBG_NEW ClientUDP(handler);
	}

	std::string NetworkUtils::GetLocalAddress()
	{
		ISocketUDP* socket = PlatformNetworkUtils::CreateSocketUDP();
		std::string address = ADDRESS_LOOPBACK;
		if (socket)
		{
			if (socket->Connect(address, 9))
			{
				address = socket->GetAddress();
			}
		}
		delete socket;
		return address;
	}
}