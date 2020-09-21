#ifdef LAMBDA_PLATFORM_MACOS
#include "Networking/Mac/MacNetworkUtils.h"
#include "Networking/Mac/MacSocketTCP.h"
#include "Networking/Mac/MacSocketUDP.h"
#include "Networking/Mac/MacIPAddress.h"

namespace LambdaEngine
{
	bool MacNetworkUtils::Init()
	{
		return NetworkUtils::Init();
	}

	void MacNetworkUtils::PreRelease()
	{
		NetworkUtils::PreRelease();
	}

	void MacNetworkUtils::PostRelease()
	{
		NetworkUtils::PostRelease();
	}

	ISocketTCP* MacNetworkUtils::CreateSocketTCP()
	{
		return new MacSocketTCP();
	}

	ISocketUDP* MacNetworkUtils::CreateSocketUDP()
	{
        return new MacSocketUDP();
	}

	IPAddress* MacNetworkUtils::CreateIPAddress(const std::string& address, uint64 hash)
	{
		return DBG_NEW MacIPAddress(address, hash);
	}
}
#endif
