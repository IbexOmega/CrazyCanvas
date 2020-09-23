#pragma once

#include "Networking/API/ServerBase.h"

#include "Networking/API/UDP/PacketManagerUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ClientRemoteUDP;

	class LAMBDA_API ServerUDP : public ServerBase
	{
		friend class ClientRemoteUDP;
		friend class NetworkUtils;

	public:
		~ServerUDP();

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ServerUDP(const ServerDesc& desc);

		virtual ISocket* SetupSocket(std::string& reason) override;
		virtual void RunReceiver() override;

	private:
		ClientRemoteUDP* GetOrCreateClient(const IPEndPoint& sender, bool& newConnection);

	private:
		PacketTransceiverUDP m_Transciver;
	};
}