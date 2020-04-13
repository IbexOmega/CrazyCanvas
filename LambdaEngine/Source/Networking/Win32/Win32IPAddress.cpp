#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Networking/Win32/Win32IPAddress.h"

#include <Ws2tcpip.h>

#include "Log/Log.h"

namespace LambdaEngine
{
	Win32IPAddress::Win32IPAddress(const std::string& address, uint64 hash) :
		IPAddress(address, hash)
	{
		if (address == ADDRESS_ANY)
		{
			m_Addr.s_addr = INADDR_ANY;
		}
		else if (address == ADDRESS_BROADCAST)
		{
			m_Addr.s_addr = INADDR_BROADCAST;
		}
		else if (address == ADDRESS_LOOPBACK)
		{
			m_Addr.s_addr = INADDR_LOOPBACK;
		}
		else if (!inet_pton(AF_INET, address.c_str(), &m_Addr))
		{
			LOG_ERROR("[Win32IPAddress]: Faild to convert [%s] to a valid IP-Address", address.c_str());
		}
	}

	Win32IPAddress::~Win32IPAddress()
	{

	}

	IN_ADDR* Win32IPAddress::GetWin32Addr()
	{
		return &m_Addr;
	}
}
#endif
