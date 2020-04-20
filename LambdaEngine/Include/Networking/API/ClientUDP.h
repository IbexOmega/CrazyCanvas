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

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;

		bool Connect(const IPEndPoint& ipEndPoint);

	protected:
		ClientUDP(IClientUDPHandler* pHandler, uint16 packets, uint8 maximumTries);

		virtual void OnPacketDelivered(NetworkPacket* pPacket) override;
		virtual void OnPacketResent(NetworkPacket* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkPacket* pPacket, uint8 tries) override;

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void SendConnectRequest();
		void SendDisconnectRequest();
		void HandleReceivedPacket(NetworkPacket* pPacket);
		void TransmitPackets();

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		IClientUDPHandler* m_pHandler;
		EClientState m_State;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];

	public:
		static ClientUDP* Create(IClientUDPHandler* pHandler, uint16 packets, uint8 maximumTries);
	};
}