#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IServer.h"
#include "Networking/API/PacketManager.h"
#include "Networking/API/PacketTransceiver.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;
	class IServerUDPHandler;
	class IClientUDPRemoteHandler;

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

		void SetSimulatePacketLoss(float lossPercentage);

	protected:
		ServerUDP(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packetPerClient, uint8 maximumTries);

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite);
		IClientUDPRemoteHandler* CreateClientUDPHandler();
		ClientUDPRemote* GetOrCreateClient(const IPEndPoint& sender, bool& newConnection);
		void OnClientDisconnected(ClientUDPRemote* client, bool sendDisconnectPacket);
		void SendDisconnect(ClientUDPRemote* client);
		void SendServerFull(ClientUDPRemote* client);
		void SendServerNotAccepting(ClientUDPRemote* client);

	public:
		static ServerUDP* Create(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packets, uint8 maximumTries);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketTransceiver m_Transciver;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		uint16 m_PacketsPerClient;
		uint8 m_MaxClients;
		uint8 m_MaxTries;
		float m_PacketLoss;
		std::atomic_bool m_Accepting;
		IServerUDPHandler* m_pHandler;
		std::unordered_map<IPEndPoint, ClientUDPRemote*, IPEndPointHasher> m_Clients;

	private:
		static std::set<ServerUDP*> s_Servers;
		static SpinLock s_Lock;
	};
}