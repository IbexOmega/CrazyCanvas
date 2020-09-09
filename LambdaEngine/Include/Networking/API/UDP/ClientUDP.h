#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/ClientBase.h"
#include "Networking/API/IPacketListener.h"

#include "Networking/API/UDP/PacketManagerUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"

namespace LambdaEngine
{
	class IClientHandler;
	class ISocketUDP;

	struct ClientUDPDesc : public PacketManagerDesc
	{
		IClientHandler* Handler = nullptr;
	};

	class LAMBDA_API ClientUDP : public ClientBase
	{
		friend class NetworkUtils;

	public:
		~ClientUDP();

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
		ClientUDP(const ClientUDPDesc& desc);

		virtual PacketManagerBase* GetPacketManager() override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

		virtual bool OnThreadsStarted() override;
		virtual void RunTransmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

		virtual void Tick(Timestamp delta) override;

	private:
		void SendConnectRequest();
		void SendDisconnectRequest();
		void HandleReceivedPacket(NetworkSegment* pPacket);
		void TransmitPackets();

	public:
		static ClientUDP* Create(const ClientUDPDesc& desc);

	private:
		ISocketUDP* m_pSocket;
		PacketTransceiverUDP m_Transciver;
		PacketManagerUDP m_PacketManager;
		SpinLock m_Lock;
		IClientHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_SendDisconnectPacket;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}