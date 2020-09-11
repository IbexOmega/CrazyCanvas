#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/TCP/ClientRemoteTCP.h"
#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/TCP/ServerTCP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ServerTCP::ServerTCP(const ServerDesc& desc) : ServerBase(desc)
	{
		
	}

	ServerTCP::~ServerTCP()
	{
		
	}

	ISocket* ServerTCP::SetupSocket()
	{
		ISocketTCP* pSocket = PlatformNetworkUtils::CreateSocketTCP();
		if (pSocket)
		{
			if (pSocket->Bind(GetEndPoint()))
			{
				if (pSocket->Listen())
				{
					LOG_INFO("[ServerTCP]: Started %s", GetEndPoint().ToString().c_str());
					return pSocket;
				}
				LOG_ERROR("[ServerTCP]: Failed To Listen To Socket");
				return nullptr;
			}
			LOG_ERROR("[ServerTCP]: Failed To Bind Socket");
			return nullptr;
		}
		LOG_ERROR("[ServerTCP]: Failed To Create Socket");
		return nullptr;
	}

	void ServerTCP::RunReceiver()
	{
		while (!ShouldTerminate())
		{
			ISocketTCP* socket = ((ISocketTCP*)m_pSocket)->Accept();
			if (!socket)
				continue;

			ClientRemoteTCP* pClient = CreateClient(socket);

			HandleNewConnection(pClient);
		}
	}

	ClientRemoteTCP* ServerTCP::CreateClient(ISocketTCP* socket)
	{
		return DBG_NEW ClientRemoteTCP(GetDescription().PoolSize, socket, this);
	}
}