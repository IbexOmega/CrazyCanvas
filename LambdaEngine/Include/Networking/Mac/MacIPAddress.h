#pragma once

#include "Networking/API/IPAddress.h"

#include <WinSock2.h>

namespace LambdaEngine
{
	class LAMBDA_API MacIPAddress : public IPAddress
	{
		friend class MacNetworkUtils;

	public:
		virtual ~MacIPAddress();

		IN_ADDR* GetMacAddr();

	private:
		MacIPAddress(const std::string& address, uint64 hash);

	private:
		IN_ADDR m_Addr;
	};
}
