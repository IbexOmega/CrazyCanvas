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
		virtual ~ClientRemoteTCP();

	protected:
		ClientRemoteTCP(const ClientRemoteDesc& desc, ISocketTCP* pSocket);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;
		virtual PacketTransceiverBase* GetTransceiver() override;

		virtual bool CanDeleteNow() override;
		virtual bool OnTerminationRequested() override;

	private:
		void RunReceiver();

	private:
		PacketManagerTCP m_PacketManager;
		PacketTransceiverTCP m_Transceiver;
		Thread* m_pThreadReceiver;
		ISocketTCP* m_pSocket;
		std::atomic_bool m_ThreadTerminated;
	};
}