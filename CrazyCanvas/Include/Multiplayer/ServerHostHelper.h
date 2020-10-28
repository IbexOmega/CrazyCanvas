#pragma once

#include "Types.h"

class ServerHostHelper
{
public:

	static void SetClientHostID(int32 serverHostID);
	static void SetAuthenticationID(int32 clientHostID);

	static int32 GetClientHostID();
	static int32 GetAuthenticationHostID();

private:
	static int32 m_sClientHostID;
	static int32 m_sAuthenticationID;
};
