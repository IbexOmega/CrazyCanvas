#pragma once

#include "ClientBase.h"
#include "ISocketUDP.h"
#include "IClientUDPHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API ClientUDP : public ClientBase
	{
	public:
		ClientUDP(const std::string& address, uint16 port, IClientUDPHandler* handler);
		~ClientUDP();

	protected:
		virtual void OnTransmitterStarted() override;
		virtual void OnReceiverStarted() override;
		virtual void UpdateReceiver(NetworkPacket* packet) override;
		virtual void OnThreadsTerminated() override;
		virtual void OnReleaseRequested() override;
		virtual bool TransmitPacket(NetworkPacket* packet) override;

	private:
		ISocketUDP* m_pSocket;
		SpinLock m_LockStart;
		IClientUDPHandler* m_pClientHandler;
	};
}
