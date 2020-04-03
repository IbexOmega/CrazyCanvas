#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <vector>
#include <atomic>
#include "IClientTCPHandler.h"

namespace LambdaEngine
{
	class IServerTCPHandler;
	class ISocketTCP;
	class Thread;

	class LAMBDA_API ServerTCP : protected IClientTCPHandler
	{
		friend class ClientTCP;

	public:
		ServerTCP(IServerTCPHandler* handler);
		~ServerTCP();

		/*
		* Starts the server on the given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		bool Start(const std::string& address, uint16 port);

		/*
		* Tells the server to stop
		*/
		void Stop();

		bool IsRunning() const;

	protected:
		virtual void OnClientConnected(ClientTCP* client) override;
		virtual void OnClientDisconnected(ClientTCP* client) override;
		virtual void OnClientFailedConnecting(ClientTCP* client) override;
		virtual void OnClientPacketReceived(ClientTCP* client, NetworkPacket* packet) override;

	private:
		void Run(std::string address, uint16 port);
		void OnStopped();

	private:
		static ISocketTCP* CreateServerSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pServerSocket;
		Thread* m_pThread;
		std::vector<ClientTCP*> m_Clients;
		std::atomic_bool m_Stop;
		IServerTCPHandler* m_pHandler;
	};
}
