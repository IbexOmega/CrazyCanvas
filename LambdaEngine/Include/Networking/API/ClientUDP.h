#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketManager.h"
#include "Networking/API/IPacketListener.h"

namespace LambdaEngine
{
	class IClientUDPHandler;
	class ISocketUDP;

	class LAMBDA_API ClientUDP :
		public NetWorker,
		public IClientUDP,
		protected IPacketListener
	{
	public:
		~ClientUDP();

		virtual bool Connect(const IPEndPoint& ipEndPoint) override;
		virtual void Disconnect() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket() override;
		virtual EClientState GetState() const override;

	protected:
		ClientUDP(IClientUDPHandler* pHandler, uint16 packets);

		virtual void OnPacketDelivered(NetworkPacket* packet) override;
		virtual void OnPacketResent(NetworkPacket* packet) override;

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTurminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void SendConnectRequest();
		void HandleReceivedPacket(NetworkPacket* pPacket);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		IClientUDPHandler* m_pHandler;
		EClientState m_State;

	public:
		static ClientUDP* Create(IClientUDPHandler* pHandler, uint16 packets);
	};
}