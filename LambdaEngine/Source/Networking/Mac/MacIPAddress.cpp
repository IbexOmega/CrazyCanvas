#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Networking/Mac/MacIPAddress.h"

#include <arpa/inet.h>

namespace LambdaEngine
{
	MacIPAddress::MacIPAddress(const std::string& address, uint64 hash) :
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
			LOG_ERROR("[MacIPAddress]: Faild to convert [%s] to a valid IP-Address", address.c_str());
			m_IsValid = false;
		}
	}

	MacIPAddress::~MacIPAddress()
	{

	}

	struct in_addr* MacIPAddress::GetMacAddr()
	{
		return &m_Addr;
	}
}
#endif
