#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketManager.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiver.h"

namespace LambdaEngine
{
	class IClientUDPHandler;
	class ISocketUDP;

	struct ClientUDPDesc : public PacketManagerDesc
	{
		IClientUDPHandler* Handler = nullptr;
	};

	class LAMBDA_API ClientUDP :
		public NetWorker,
		public IClientUDP,
		protected IPacketListener
	{
		friend class NetworkUtils;

	public:
		~ClientUDP();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;

		bool Connect(const IPEndPoint& ipEndPoint);

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ClientUDP(const ClientUDPDesc& desc);

		virtual PacketManager* GetPacketManager() override;

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
		void Tick(Timestamp delta);

	public:
		static ClientUDP* Create(const ClientUDPDesc& desc);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketUDP* m_pSocket;
		PacketTransceiver m_Transciver;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		IClientUDPHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_SendDisconnectPacket;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];

	private:
		static std::set<ClientUDP*> s_Clients;
		static SpinLock s_Lock;
	};
}