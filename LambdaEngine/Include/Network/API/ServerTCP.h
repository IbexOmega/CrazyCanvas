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

		bool Start(const std::string& address, uint16 port);
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
