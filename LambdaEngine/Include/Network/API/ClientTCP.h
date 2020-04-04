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
		~ClientTCP();
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
		* Sends a packet Immediately using the current thread
		*/
		bool SendPacketImmediately(NetworkPacket* packet);

		/*
		* return - true if this instance is on the server side, otherwise false.
		*/
		bool IsServerSide() const;

		/*
		* Release all the resouces used by the client and will be deleted when each thread has terminated.
		*/
		void Release();

	private:
		ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket);

		void Run(std::string address, uint16 port);
		void OnStopped();
		bool Receive(char* buffer, int bytesToRead);
		bool ReceivePacket(NetworkPacket* packet);

		void RunTransmit();
		void OnStoppedSend();
		bool Transmit(char* buffer, int bytesToSend);
		void TransmitPackets(std::queue<NetworkPacket*>* packets);

		static void DeletePackets(std::queue<NetworkPacket*>* packets);
		static void DeletePacket(NetworkPacket* packet);

		static ISocketTCP* CreateClientSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pSocket;
		IClientTCPHandler* m_pHandler;
		Thread* m_pThread;
		Thread* m_pThreadSend;
		bool m_ServerSide;
		std::atomic_bool m_Stop;
		std::atomic_bool m_Release;
		std::queue<NetworkPacket*> m_PacketsToSend;
		SpinLock m_LockPacketsToSend;
	};
}
