#pragma once

#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/UDP/PacketManagerUDP.h"

namespace LambdaEngine
{
	class ServerUDP;
	class PacketTransceiverUDP;

	class LAMBDA_API ClientRemoteUDP : public ClientRemoteBase
	{
		friend class ServerUDP;
		
	protected:
		ClientRemoteUDP(const ClientRemoteDesc& desc, const IPEndPoint& ipEndPoint, PacketTransceiverUDP* pTransceiver);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;
		virtual PacketTransceiverBase* GetTransceiver() override;

	private:
		PacketManagerUDP m_PacketManager;
		PacketTransceiverUDP* m_pTransceiver;
	};
}