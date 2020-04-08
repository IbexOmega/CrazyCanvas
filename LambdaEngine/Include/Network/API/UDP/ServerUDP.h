#pragma once

#include "../ClientBase.h"
#include "../IClient.h"
#include "IServerUDPHandler.h"
#include <unordered_map>

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;

	class LAMBDA_API ServerUDP : protected ClientBase<IClient>
	{
		friend class ClientUDPRemote;
		friend class NetworkUtils;

	public:
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

		/*
		* Release all the resouces used by the client and will be deleted when each thread has terminated.
		*/
		virtual void Release() override;

	protected:
		virtual void OnThreadsStarted() override;
		virtual void OnThreadsStartedPost() override;
		virtual void UpdateReceiver(NetworkPacket* packet) override;
		virtual void OnThreadsTerminated() override;
		virtual void OnReleaseRequested() override;
		virtual bool TransmitPacket(NetworkPacket* packet) override;

	private:
		ServerUDP(IServerUDPHandler* handler);

		void OnClientReleased(ClientUDPRemote* client);

	private:
		static ISocketUDP* CreateServerSocket(const std::string& address, uint16 port);
		static uint64 Hash(const std::string& address, uint64 port);

	private:
		SpinLock m_Lock;
		ISocketUDP* m_pServerSocket;
		IServerUDPHandler* m_pHandler;
		std::unordered_map<uint64, ClientUDPRemote*> m_Clients;
		char m_ReceiveBuffer[MAXIMUM_DATAGRAM_SIZE];
	};
}
