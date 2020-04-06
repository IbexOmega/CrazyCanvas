#include "Network/API/SocketFactory.h"
#include "Network/API/ClientTCP.h"
#include "Network/API/ISocketUDP.h"
#include "Network/API/PlatformSocketFactory.h"

namespace LambdaEngine
{
	bool SocketFactory::Init()
	{
		ClientTCP::InitStatic();
		return true;
	}

	void SocketFactory::Tick(Timestamp dt)
	{
		ClientTCP::TickStatic(dt);
	}

	void SocketFactory::Release()
	{
		ClientTCP::ReleaseStatic();
	}

	std::string SocketFactory::GetLocalAddress()
	{
		ISocketUDP* socket = PlatformSocketFactory::CreateSocketUDP();
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