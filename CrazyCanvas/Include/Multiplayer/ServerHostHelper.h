#pragma once

#include "Types.h"

class ServerHostHelper
{
	friend class CrazyCanvas;

public:
	static void SetClientHostID(int32 serverHostID);
	static void SetAuthenticationID(int32 clientHostID);

	static int32 GetClientHostID();
	static int32 GetAuthenticationHostID();

private:
	static void Init();

private:
	static int32 s_ClientHostID;
	static int32 s_AuthenticationID;
};
