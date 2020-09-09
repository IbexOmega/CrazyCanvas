#pragma once

#include "Networking/API/IClient.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class IClientRemoteHandler;

	class LAMBDA_API ClientRemoteBase : public IClient
	{
		friend class ServerBase;

	public:
		ClientRemoteBase();

	protected:
		virtual void Tick(Timestamp delta) = 0;

	protected:
		SpinLock m_Lock;
		IClientRemoteHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_Release;
		bool m_DisconnectedByRemote;
	};
}