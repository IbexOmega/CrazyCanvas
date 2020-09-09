#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/ServerBase.h"

#include "Networking/API/TCP/PacketManagerTCP.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketTCP;
	class ClientTCPRemote;
	class IServerHandler;
	class IClientRemoteHandler;

	struct ServerTCPDesc : public PacketManagerDesc
	{
		IServerHandler* Handler	= nullptr;
		uint8 MaxClients		= 1;
	};

	class LAMBDA_API ServerTCP : public ServerBase
	{
		friend class ClientTCPRemote;
		friend class NetworkUtils;

	public:
		~ServerTCP();

	protected:
		ServerTCP(const ServerTCPDesc& desc);

		virtual ISocket* SetupSocket() override;
		virtual void RunReceiver() override;
		virtual void TransmitPacketsForClient(ClientRemoteBase* pClient) override;

	private:
		IClientRemoteHandler* CreateClientHandler();
		ClientTCPRemote* CreateClient(ISocketTCP* socket);
		void OnClientDisconnected(ClientTCPRemote* client, bool sendDisconnectPacket);
		void SendDisconnect(ClientTCPRemote* client);
		void SendServerFull(ClientTCPRemote* client);
		void SendServerNotAccepting(ClientTCPRemote* client);

	public:
		static ServerTCP* Create(const ServerTCPDesc& desc);

	private:
		ServerTCPDesc m_Desc;
		float m_PacketLoss;
	};
}