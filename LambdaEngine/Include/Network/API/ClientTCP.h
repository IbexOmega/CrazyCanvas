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
		
		bool Connect(const std::string& address, uint16 port);
		void Disconnect();
		void SendPacket(NetworkPacket* packet);
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
