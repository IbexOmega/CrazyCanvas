#pragma once

#include "Networking/API/ClientBase.h"

#include "Networking/API/TCP/PacketManagerTCP.h"
#include "Networking/API/TCP/PacketTransceiverTCP.h"

namespace LambdaEngine
{
	class LAMBDA_API ClientTCP : public ClientBase
	{
		friend class NetworkUtils;

	public:
		~ClientTCP();

	protected:
		ClientTCP(const ClientDesc& desc);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;
		virtual PacketTransceiverBase* GetTransceiver() override;
		virtual ISocket* SetupSocket() override;
		virtual void RunReceiver() override;
		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

	private:
		PacketTransceiverTCP m_Transceiver;
		PacketManagerTCP m_PacketManager;
	};
}