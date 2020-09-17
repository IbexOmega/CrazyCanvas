#pragma once

#include "Networking/API/ClientBase.h"
#include "Networking/API/UDP/PacketManagerUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"

namespace LambdaEngine
{
	class LAMBDA_API ClientUDP : public ClientBase
	{
		friend class NetworkUtils;

	public:
		~ClientUDP();

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ClientUDP(const ClientDesc& desc);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;
		virtual PacketTransceiverBase* GetTransceiver() override;
		virtual ISocket* SetupSocket(std::string& reason) override;
		virtual void RunReceiver() override;
		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

	private:
		PacketTransceiverUDP m_Transciver;
		PacketManagerUDP m_PacketManager;
	};
}