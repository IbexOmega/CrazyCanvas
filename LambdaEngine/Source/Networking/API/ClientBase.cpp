#include "Networking/API/ClientBase.h"

namespace LambdaEngine
{
	std::set<ClientBase*> ClientBase::s_Clients;
	SpinLock ClientBase::s_Lock;

	ClientBase::ClientBase()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.insert(this);
	}

	ClientBase::~ClientBase()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.erase(this);
	}

	void ClientBase::FixedTickStatic(Timestamp timestamp)
	{
		if (!s_Clients.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ClientBase* client : s_Clients)
			{
				client->Tick(timestamp);
			}
		}
	}
}
