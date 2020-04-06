#pragma once

#include "ServerBase.h"
#include "IRemoteClientTCPHandler.h"

namespace LambdaEngine
{
	class ISocketTCP;
	class ClientTCP;
	class IServerTCPHandler;

	class LAMBDA_API ServerTCP : public ServerBase, protected IRemoteClientTCPHandler
	{
		friend class ClientTCP;

	public:
		ServerTCP(uint16 maxClients, IServerTCPHandler* handler);
		~ServerTCP();

		/*
		* return - The number of connected clients
		*/
		uint8 GetNrOfClients() const;

	protected:
		virtual void OnThreadStarted() override;
		virtual void OnThreadUpdate() override;
		virtual void OnThreadTerminated() override;
		virtual void OnStopRequested() override;
		virtual void OnReleaseRequested() override;

		virtual void OnClientDisconnected(ClientTCP* client) override;

	private:
		void HandleNewClient(ClientTCP* client);
		void AddClient(ClientTCP* client);
		void RemoveClient(ClientTCP* client);

	private:
		static ISocketTCP* CreateServerSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pServerSocket;
		std::vector<ClientTCP*> m_Clients;
		IServerTCPHandler* m_pHandler;
		SpinLock m_Lock;
		uint16 m_MaxClients;
	};
}
