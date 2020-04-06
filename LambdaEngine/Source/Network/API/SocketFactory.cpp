#include "Network/API/SocketFactory.h"
#include "Network/API/ClientTCP.h"

namespace LambdaEngine
{
	bool SocketFactory::Init()
	{
		ClientTCP::InitStatic();
		return true;
	}

	void SocketFactory::Tick(Timestamp dt)
	{
		ClientTCP::TickStatic(dt);
	}

	void SocketFactory::Release()
	{
		ClientTCP::ReleaseStatic();
	}
}