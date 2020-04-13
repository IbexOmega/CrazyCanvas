#pragma once

#include "Networking/API/IPAddress.h"

#include <WinSock2.h>

namespace LambdaEngine
{
	class LAMBDA_API Win32IPAddress : public IPAddress
	{
		friend class Win32NetworkUtils;

	public:
		virtual ~Win32IPAddress();

		IN_ADDR* GetWin32Addr();

	private:
		Win32IPAddress(const std::string& address, uint64 hash);

	private:
		IN_ADDR m_Addr;
	};
}
