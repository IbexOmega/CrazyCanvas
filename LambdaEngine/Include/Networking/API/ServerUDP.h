#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IServer.h"
#include "Networking/API/PacketTransceiver.h"
#include "Networking/API/PacketManager.h"
#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;
	class IServerUDPHandler;
	class IClientUDPRemoteHandler;

	struct ServerUDPDesc : public PacketManagerDesc
	{
		IServerUDPHandler* Handler	= nullptr;
		uint8 MaxClients			= 1;
	};

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

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ServerUDP(const ServerUDPDesc& desc);

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
		void Tick(Timestamp delta);

	public:
		static ServerUDP* Create(const ServerUDPDesc& desc);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketTransceiver m_Transciver;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		ServerUDPDesc m_Desc;
		float m_PacketLoss;
		std::atomic_bool m_Accepting;
		std::unordered_map<IPEndPoint, ClientUDPRemote*, IPEndPointHasher> m_Clients;

	private:
		static std::set<ServerUDP*> s_Servers;
		static SpinLock s_Lock;
	};
}