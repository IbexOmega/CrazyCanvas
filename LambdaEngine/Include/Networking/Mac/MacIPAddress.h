#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Networking/API/IPAddress.h"

#include <netinet/in.h>

namespace LambdaEngine
{
	class LAMBDA_API MacIPAddress : public IPAddress
	{
		friend class MacNetworkUtils;

	public:
		virtual ~MacIPAddress();

		struct in_addr* GetMacAddr();

	private:
		MacIPAddress(const std::string& address, uint64 hash);

	private:
		struct in_addr m_Addr;
	};
}
#endif
