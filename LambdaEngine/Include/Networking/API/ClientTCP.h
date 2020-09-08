#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClient.h"
#include "Networking/API/PacketManagerUDP.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiverUDP.h"

namespace LambdaEngine
{
	class IClientHandler;
	class ISocketUDP;

	struct ClientTCPDesc : public PacketManagerDesc
	{
		IClientHandler* Handler = nullptr;
	};

	class LAMBDA_API ClientTCP :
		public NetWorker,
		public IClient,
		protected IPacketListener
	{
		friend class NetworkUtils;

	public:
		~ClientTCP();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkSegment* packet) override;
		virtual bool SendReliable(NetworkSegment* packet, IPacketListener* listener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;

		bool Connect(const IPEndPoint& ipEndPoint);

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ClientTCP(const ClientTCPDesc& desc);

		virtual PacketManagerUDP* GetPacketManager() override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

		virtual bool OnThreadsStarted() override;
		virtual void RunTransmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		void SendConnectRequest();
		void SendDisconnectRequest();
		void HandleReceivedPacket(NetworkSegment* pPacket);
		void TransmitPackets();
		void Tick(Timestamp delta);

	public:
		static ClientTCP* Create(const ClientTCPDesc& desc);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		ISocketUDP* m_pSocket;
		PacketTransceiverUDP m_Transciver;
		PacketManagerUDP m_PacketManager;
		SpinLock m_Lock;
		IClientHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_SendDisconnectPacket;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];

	private:
		static std::set<ClientTCP*> s_Clients;
		static SpinLock s_Lock;
	};
}