#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <atomic>

namespace LambdaEngine
{
	class ISocketTCP;
	class IClientTCPHandler;
	class Thread;

	class LAMBDA_API ClientTCP
	{
		friend class ServerTCP;

	public:
		ClientTCP(IClientTCPHandler* handler);
		
		bool Connect(const std::string& address, uint16 port);
		void Disconnect();
		void SendPacket(const std::string& data);
		bool IsServerSide() const;

	private:
		ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket);

		void Run(std::string address, uint16 port);
		void OnStoped();

		static ISocketTCP* CreateClientSocket(const std::string& address, uint16 port);

	private:
		ISocketTCP* m_pSocket;
		IClientTCPHandler* m_pHandler;
		Thread* m_pThread;
		bool m_ServerSide;
		std::atomic_bool m_Stop;
	};
}
