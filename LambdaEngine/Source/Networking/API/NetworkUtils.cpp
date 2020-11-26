#include "Networking/API/NetworkUtils.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/ServerBase.h"
#include "Networking/API/ClientBase.h"

#include "Networking/API/NetworkDebugger.h"

#include "Networking/API/UDP/ISocketUDP.h"

namespace LambdaEngine
{
	bool NetworkUtils::Init()
	{
		IPAddress::InitStatic();
		NetworkDebugger::Init();
		return true;
	}

	void NetworkUtils::Tick(Timestamp dt)
	{
		UNREFERENCED_VARIABLE(dt);
	}

	void NetworkUtils::FixedTick(Timestamp dt)
	{
		ServerBase::FixedTickStatic(dt);
		ClientBase::FixedTickStatic(dt);
		NetworkDiscovery::FixedTickStatic(dt);
		NetWorker::FixedTickStatic(dt);
		ClientRemoteBase::FixedTickStatic(dt);
	}

	void NetworkUtils::PreRelease()
	{
		NetworkDiscovery::ReleaseStatic();
	}

	void NetworkUtils::PostRelease()
	{
		NetWorker::ReleaseStatic();
		ClientRemoteBase::ReleaseStatic();
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

	ClientBase* NetworkUtils::CreateClient(const ClientDesc& desc)
	{
		if(desc.Protocol == EProtocol::TCP)
			return DBG_NEW ClientTCP(desc);
		else if (desc.Protocol == EProtocol::UDP)
			return DBG_NEW ClientUDP(desc);

		LOG_ERROR_CRIT("What kind of protocol is this ?");
		return nullptr;
	}

	ServerBase* NetworkUtils::CreateServer(const ServerDesc& desc)
	{
		if (desc.Protocol == EProtocol::TCP)
			return DBG_NEW ServerTCP(desc);
		else if (desc.Protocol == EProtocol::UDP)
			return DBG_NEW ServerUDP(desc);

		LOG_ERROR_CRIT("What kind of protocol is this ?");
		return nullptr;
	}
}