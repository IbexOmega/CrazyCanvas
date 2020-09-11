#pragma once

#include "Networking/API/ClientRemoteBase.h"

#include "Networking/API/TCP/PacketManagerTCP.h"
#include "Networking/API/TCP/PacketTransceiverTCP.h"

namespace LambdaEngine
{
	class ServerTCP;
	class ISocketTCP;
	class Thread;

	class LAMBDA_API ClientRemoteTCP : public ClientRemoteBase
	{
		friend class ServerTCP;

	public:
		virtual void Release() override;

	protected:
		ClientRemoteTCP(uint16 packetPoolSize, ISocketTCP* pSocket, ServerTCP* pServer);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;
		virtual PacketTransceiverBase* GetTransceiver() override;

	private:
		void RunReceiver();

	private:
		PacketManagerTCP m_PacketManager;
		PacketTransceiverTCP m_Transceiver;
		Thread* m_pThreadReceiver;
		ISocketTCP* m_pSocket;
	};
}