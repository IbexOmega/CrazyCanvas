#include "SocketFactory.h"
#include "ClientTCP.h"

namespace LambdaEngine
{
	bool SocketFactory::Init()
	{
		ClientTCP::Init();
		return true;
	}

	void SocketFactory::Tick(Timestamp dt)
	{
		ClientTCP::Tick(dt);
	}

	void SocketFactory::Release()
	{
		ClientTCP::ReleaseStatic();
	}
}