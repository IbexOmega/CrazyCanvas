#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IServer.h"
#include "Networking/API/PacketManager.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;
	class IServerUDPHandler;
	class IClientUDPHandler;

	class LAMBDA_API ServerUDP : public NetWorker, public IServer
	{
		friend class ClientUDPRemote;
		friend class NetworkUtils;

	public:
		~ServerUDP();

		virtual bool Start(const IPEndPoint& ipEndPoint) override;
		virtual void Stop() override;
		virtual void Release() override;
		virtual bool IsRunning() override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual void SetAcceptingConnections(bool accepting) override;
		virtual bool IsAcceptingConnections() override;

	protected:
		ServerUDP(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packetPerClient);

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite);
		IClientUDPHandler* CreateClientUDPHandler();
		void OnClientDisconnected(ClientUDPRemote* client, bool sendDisconnectPacket);
		void SendDisconnect(ClientUDPRemote* client);
		void SendServerFull(ClientUDPRemote* client);
		void SendServerNotAccepting(ClientUDPRemote* client);

	public:
		static ServerUDP* Create(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packets);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		uint16 m_PacketsPerClient;
		uint8 m_MaxClients;
		std::atomic_bool m_Accepting;
		IServerUDPHandler* m_pHandler;
		std::unordered_map<IPEndPoint, ClientUDPRemote*, IPEndPointHasher> m_Clients;

	private:
		static std::set<ServerUDP*> s_Servers;
		static SpinLock s_Lock;
	};
}