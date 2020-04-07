#pragma once

#include "../ClientBase.h"
#include "ISocketUDP.h"
#include "IClientUDP.h"
#include "IClientUDPHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API ClientUDP : public ClientBase<IClientUDP>
	{
		friend class NetworkUtils;

	public:
		~ClientUDP();
		
		/*
		* Start the client and set the given ip-address and port. To send to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Start(const std::string& address, uint16 port) override final;

	protected:
		ClientUDP(IClientUDPHandler* clientHandler);

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
		char m_ReceiveBuffer[MAXIMUM_DATAGRAM_SIZE];
	};
}