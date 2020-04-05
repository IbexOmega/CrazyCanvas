#pragma once

#include "Defines.h"
#include "Types.h"

#include "Threading/SpinLock.h"

#include "Time/API/Timestamp.h"

#include "NetworkPacket.h"
#include "IClientUDPHandler.h"

#include <string>
#include <atomic>
#include <queue>
#include <set>

namespace LambdaEngine
{
	class ISocketUDP;
	class Thread;
	class NetworkPacket;

	class LAMBDA_API ClientUDP
	{
		friend class ServerTCP;
		friend class SocketFactory;

	public:
		ClientUDP(const std::string& address, uint16 port, IClientUDPHandler* handler);
		~ClientUDP();

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

		int32 GetBytesSent() const;
		int32 GetBytesReceived() const;
		int32 GetPacketsSent() const;
		int32 GetPacketsReceived() const;

	private:

		void Run(std::string address, uint16 port);
		void OnStopped();

		void RunTransmit();
		void OnStoppedSend();
		bool Transmit(char* buffer, int bytesToSend);
		void TransmitPackets(std::queue<NetworkPacket*>* packets);

	private:
		static void DeletePackets(std::queue<NetworkPacket*>* packets);
		static void DeletePacket(NetworkPacket* packet);
		static ISocketUDP* CreateClientSocket();

	private:
		ISocketUDP* m_pSocket;
		IClientUDPHandler* m_pHandler;
		Thread* m_pThread;
		Thread* m_pThreadSend;
		bool m_ServerSide;
		std::atomic_bool m_Stop;
		std::atomic_bool m_Release;
		std::queue<NetworkPacket*> m_PacketsToSend;
		SpinLock m_LockPacketsToSend;
		std::string m_Address;
		uint16 m_Port;

		uint32 m_NrOfPingTransmitted;
		uint32 m_NrOfPingReceived;
		uint32 m_NrOfPacketsTransmitted;
		uint32 m_NrOfPacketsReceived;
		uint32 m_NrOfBytesTransmitted;
		uint32 m_NrOfBytesReceived;
	};
}
