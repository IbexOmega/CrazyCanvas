#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <vector>
#include <atomic>
#include "IClientTCPHandler.h"
#include "Threading/SpinLock.h"

namespace LambdaEngine
{
	class IServerTCPHandler;
	class ISocketTCP;
	class Thread;

	class LAMBDA_API ServerTCP : protected IClientTCPHandler
	{
		friend class ClientTCP2;

	public:
		ServerTCP(uint16 maxClients, IServerTCPHandler* handler);
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

		/*
		* return - The currently used inet address.
		*/
		const std::string& GetAddress();

		/*
		* return - The currently used port.
		*/
		uint16 GetPort();

		/*
		* return - The number of connected clients
		*/
		uint8 GetNrOfClients() const;

	protected:
		virtual void OnClientConnected(ClientTCP2* client) override;
		virtual void OnClientDisconnected(ClientTCP2* client) override;
		virtual void OnClientFailedConnecting(ClientTCP2* client) override;
		virtual void OnClientPacketReceived(ClientTCP2* client, NetworkPacket* packet) override;

	private:
		void Run(std::string address, uint16 port);
		void OnStopped();

		void HandleNewClient(ClientTCP2* client);
		void AddClient(ClientTCP2* client);
		void RemoveClient(ClientTCP2* client);
		void ClearClients();

	private:
		static ISocketTCP* CreateServerSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pServerSocket;
		Thread* m_pThread;
		std::vector<ClientTCP2*> m_Clients;
		std::atomic_bool m_Stop;
		IServerTCPHandler* m_pHandler;
		SpinLock m_Lock;
		uint16 m_MaxClients;
	};
}
