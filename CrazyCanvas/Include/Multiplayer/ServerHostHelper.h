#pragma once

#include "Types.h"

class ServerHostHelper
{
public:

	static void SetServerHostID(int32 serverHostID);
	static void SetClientHostID(int32 clientHostID);

	static int32 GetServerHostID();
	static int32 GetClientHostID();

private:
	static int32 m_sServerHostID;
	static int32 m_sClientHostID;
};
