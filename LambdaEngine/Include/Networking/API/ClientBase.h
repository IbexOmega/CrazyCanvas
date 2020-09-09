#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClient.h"
#include "Networking/API/IPacketListener.h"

namespace LambdaEngine
{
	class IClientHandler;
	class ISocketTCP;

	class LAMBDA_API ClientBase :
		public NetWorker,
		public IClient,
		protected IPacketListener
	{
		friend class NetworkUtils;

	public:
		DECL_UNIQUE_CLASS(ClientBase);

		ClientBase();
		virtual ~ClientBase();

	protected:
		virtual void Tick(Timestamp delta) = 0;

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		static std::set<ClientBase*> s_Clients;
		static SpinLock s_Lock;
	};
}