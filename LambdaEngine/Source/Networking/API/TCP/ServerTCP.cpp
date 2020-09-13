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

	ISocket* ServerTCP::SetupSocket(std::string& reason)
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
				reason = "Listen Socket Failed";
				delete pSocket;
				return nullptr;
			}
			reason = "Bind Socket Failed " + GetEndPoint().ToString();
			delete pSocket;
			return nullptr;
		}
		reason = "Create Socket Failed";
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
		ClientRemoteDesc desc = {};
		memcpy(&desc, &GetDescription(), sizeof(ServerDesc));
		desc.Server = this;

		return DBG_NEW ClientRemoteTCP(desc, socket);
	}
}