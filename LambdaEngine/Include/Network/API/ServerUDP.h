#pragma once

#include "ServerBase.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class IServerUDPHandler;

	class LAMBDA_API ServerUDP : public ServerBase
	{
	public:
		ServerUDP(IServerUDPHandler* handler);
		~ServerUDP();

	protected:
		virtual void OnThreadStarted() override;
		virtual void OnThreadUpdate() override;
		virtual void OnThreadTerminated() override;
		virtual void OnStopRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		static ISocketUDP* CreateServerSocket(const std::string& address, uint16 port);

	private:
		ISocketUDP* m_pServerSocket;
		IServerUDPHandler* m_pHandler;
	};
}
