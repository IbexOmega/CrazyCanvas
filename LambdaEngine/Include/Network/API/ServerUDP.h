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
	class IServerUDPHandler;
	class ISocketUDP;
	class Thread;

	class LAMBDA_API ServerUDP
	{
	public:
		ServerUDP(IServerUDPHandler* handler);
		~ServerUDP();

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

	private:
		void Run(std::string address, uint16 port);
		void OnStopped();

		void HandleReceivedPacket(NetworkPacket* packet, const std::string& address, uint16 port);

		void HandleNewClient(ClientTCP* client);
		void AddClient(ClientTCP* client);
		void RemoveClient(ClientTCP* client);
		void ClearClients();

	private:
		static ISocketUDP* CreateServerSocket(const std::string& address, uint16 port);

	private:
		ISocketUDP* m_pServerSocket;
		Thread* m_pThread;
		//std::vector<ClientTCP*> m_Clients;
		std::atomic_bool m_Stop;
		IServerUDPHandler* m_pHandler;
		SpinLock m_Lock;
	};
}