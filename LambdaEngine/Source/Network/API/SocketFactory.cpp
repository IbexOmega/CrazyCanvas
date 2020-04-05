#include "Network/API/SocketFactory.h"
#include "Network/API/ClientTCP2.h"

namespace LambdaEngine
{
	bool SocketFactory::Init()
	{
		ClientTCP2::InitStatic();
		return true;
	}

	void SocketFactory::Tick(Timestamp dt)
	{
		ClientTCP2::TickStatic(dt);
	}

	void SocketFactory::Release()
	{
		ClientTCP2::ReleaseStatic();
	}
}