#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IServer.h"
#include "Networking/API/PacketManager.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;

	class LAMBDA_API ServerUDP : public NetWorker, public IServer
	{
		friend class ClientUDPRemote;

	public:
		~ServerUDP();

		virtual bool Start(const IPEndPoint& ipEndPoint) override;
		virtual void Stop() override;
		virtual bool IsRunning() override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual void SetAcceptingConnections(bool accepting) override;
		virtual bool IsAcceptingConnections() override;

	protected:
		ServerUDP(uint16 packetPerClient);

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTurminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		SpinLock m_Lock;
		uint16 m_PacketsPerClient;
		std::atomic_bool m_Accepting;
		std::unordered_map<IPEndPoint, ClientUDPRemote*, IPEndPointHasher> m_Clients;

	public:
		static ServerUDP* Create(uint16 packets);
	};
}