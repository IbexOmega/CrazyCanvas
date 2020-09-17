#pragma once

#include "Networking/API/ServerBase.h"
#include "Networking/API/TCP/PacketManagerTCP.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ClientRemoteTCP;

	class LAMBDA_API ServerTCP : public ServerBase
	{
		friend class ClientRemoteTCP;
		friend class NetworkUtils;

	public:
		~ServerTCP();

	protected:
		ServerTCP(const ServerDesc& desc);

		virtual ISocket* SetupSocket(std::string& reason) override;
		virtual void RunReceiver() override;

	private:
		ClientRemoteTCP* CreateClient(ISocketTCP* socket);
	};
}