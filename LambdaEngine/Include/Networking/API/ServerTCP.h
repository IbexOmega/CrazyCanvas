#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IServer.h"
#include "Networking/API/PacketTransceiverUDP.h"
#include "Networking/API/PacketManagerUDP.h"
#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketTCP;
	class ClientTCPRemote;
	class IServerHandler;
	class IClientRemoteHandler;

	struct ServerTCPDesc : public PacketManagerDesc
	{
		IServerHandler* Handler	= nullptr;
		uint8 MaxClients			= 1;
	};

	class LAMBDA_API ServerTCP : public NetWorker, public IServer
	{
		friend class ClientTCPRemote;
		friend class NetworkUtils;

	public:
		~ServerTCP();

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
		ServerTCP(const ServerTCPDesc& desc);

		virtual bool OnThreadsStarted() override;
		virtual void RunTransmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		IClientRemoteHandler* CreateClientHandler();
		ClientTCPRemote* GetOrCreateClient(const IPEndPoint& sender, bool& newConnection);
		void OnClientDisconnected(ClientTCPRemote* client, bool sendDisconnectPacket);
		void SendDisconnect(ClientTCPRemote* client);
		void SendServerFull(ClientTCPRemote* client);
		void SendServerNotAccepting(ClientTCPRemote* client);
		void Tick(Timestamp delta);

	public:
		static ServerTCP* Create(const ServerTCPDesc& desc);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketTCP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketTransceiverUDP m_Transciver;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		ServerTCPDesc m_Desc;
		float m_PacketLoss;
		std::atomic_bool m_Accepting;
		std::unordered_map<IPEndPoint, ClientTCPRemote*, IPEndPointHasher> m_Clients;

	private:
		static std::set<ServerTCP*> s_Servers;
		static SpinLock s_Lock;
	};
}