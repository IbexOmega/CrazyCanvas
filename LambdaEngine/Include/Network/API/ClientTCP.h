#pragma once

#include "Defines.h"
#include "Types.h"
#include "Threading/SpinLock.h"
#include <string>
#include <atomic>
#include <queue>

namespace LambdaEngine
{
	class ISocketTCP;
	class IClientTCPHandler;
	class Thread;
	class NetworkPacket;

	class LAMBDA_API ClientTCP
	{
		friend class ServerTCP;

	public:
		ClientTCP(IClientTCPHandler* handler);
		
		/*
		* Connects the client to a given ip-address and port. To connect to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		bool Connect(const std::string& address, uint16 port);

		/*
		* Disconnects the client
		*/
		void Disconnect();

		/*
		* Sends a packet
		*/
		void SendPacket(NetworkPacket* packet);

		/*
		* return - true if this instance is on the server side, otherwise false.
		*/
		bool IsServerSide() const;

	private:
		ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket);

		void Run(std::string address, uint16 port);
		void OnStopped();
		bool Receive(char* buffer, int bytesToRead);

		void RunSend();
		void OnStoppedSend();
		bool Send(char* buffer, int bytesToSend);

		static ISocketTCP* CreateClientSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pSocket;
		IClientTCPHandler* m_pHandler;
		Thread* m_pThread;
		Thread* m_pThreadSend;
		bool m_ServerSide;
		std::atomic_bool m_Stop;
		std::queue<NetworkPacket*> m_PacketsToSend;
		SpinLock m_LockPacketsToSend;
	};
}
