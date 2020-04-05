#pragma once

#include "Defines.h"
#include "Types.h"
#include "Threading/SpinLock.h"
#include "Time/API/Timestamp.h"
#include <string>
#include <atomic>
#include <queue>
#include <set>

namespace LambdaEngine
{
	class ISocketTCP;
	class IClientTCPHandler;
	class Thread;
	class NetworkPacket;

	class LAMBDA_API ClientTCP
	{
		friend class ServerTCP;
		friend class SocketFactory;

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

		/*
		* return - true if the client is connected.
		*/
		bool IsConnected() const;

		int32 GetBytesSent() const;
		int32 GetBytesReceived() const;
		int32 GetPacketsSent() const;
		int32 GetPacketsReceived() const;

	private:
		ClientTCP(const std::set<IClientTCPHandler*>& handlers, ISocketTCP* socket);

		void Update(Timestamp dt);
		void ResetReceiveTimer();
		void ResetTransmitTimer();

		void Run(std::string address, uint16 port);
		void OnStopped();
		bool Receive(char* buffer, int bytesToRead);
		bool ReceivePacket(NetworkPacket* packet);
		void HandlePacket(NetworkPacket* packet);

		void RunTransmit();
		void OnStoppedSend();
		bool Transmit(char* buffer, int bytesToSend);
		void TransmitPackets(std::queue<NetworkPacket*>* packets);

	private:
		static void DeletePackets(std::queue<NetworkPacket*>* packets);
		static void DeletePacket(NetworkPacket* packet);
		static void Init();
		static void Tick(Timestamp timestamp);
		static void ReleaseStatic();
		static ISocketTCP* CreateClientSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pSocket;
		std::set<IClientTCPHandler*> m_Handlers;
		Thread* m_pThread;
		Thread* m_pThreadSend;
		bool m_ServerSide;
		std::atomic_bool m_Stop;
		std::atomic_bool m_Release;
		std::queue<NetworkPacket*> m_PacketsToSend;
		SpinLock m_LockPacketsToSend;

		int64 m_TimerReceived;
		int64 m_TimerTransmit;
		uint32 m_NrOfPingTransmitted;
		uint32 m_NrOfPingReceived;
		uint32 m_NrOfPacketsTransmitted;
		uint32 m_NrOfPacketsReceived;
		uint32 m_NrOfBytesTransmitted;
		uint32 m_NrOfBytesReceived;

	private:
		static NetworkPacket s_PacketPing;
		static std::set<ClientTCP*>* s_Clients;
		static SpinLock* s_LockClients;
	};
}
