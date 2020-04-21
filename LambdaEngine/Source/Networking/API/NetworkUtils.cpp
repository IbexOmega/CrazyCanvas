#include "Networking/API/NetworkUtils.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/ClientUDP.h"

namespace LambdaEngine
{
	bool NetworkUtils::Init()
	{
		IPAddress::InitStatic();
		return true;
	}

	void NetworkUtils::Tick(Timestamp dt)
	{
		UNREFERENCED_VARIABLE(dt);
	}

	void NetworkUtils::FixedTick(Timestamp dt)
	{
		ServerUDP::FixedTickStatic(dt);
		ClientUDP::FixedTickStatic(dt);
	}

	void NetworkUtils::Release()
	{
		IPAddress::ReleaseStatic();
	}

	IPAddress* NetworkUtils::GetLocalAddress()
	{
		ISocketUDP* socket = PlatformNetworkUtils::CreateSocketUDP();
		IPEndPoint endPoint(IPAddress::LOOPBACK, 9);
		IPAddress* localAddress = nullptr;

		if (socket)
		{
			if (socket->Connect(endPoint))
			{
				localAddress = socket->GetEndPoint().GetAddress();
			}

			delete socket;
		}

		return localAddress;
	}

	IPAddress* NetworkUtils::CreateIPAddress(const std::string& address, uint64 hash)
	{
		UNREFERENCED_VARIABLE(address);
		UNREFERENCED_VARIABLE(hash);
		return nullptr;
	}

	ISocketTCP* NetworkUtils::CreateSocketTCP()
	{
		return nullptr;
	}

	ISocketUDP* NetworkUtils::CreateSocketUDP()
	{
		return nullptr;
	}
}