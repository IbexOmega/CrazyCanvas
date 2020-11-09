#pragma once

#include "Types.h"

class ServerHostHelper
{
	friend class CrazyCanvas;

public:
	static void SetClientHostID(int32 serverHostID);
	static void SetAuthenticationID(int32 clientHostID);
	static void SetIsHost(bool host);
	static void SetIsDedicated(bool dedicated);

	static int32 GetClientHostID();
	static int32 GetAuthenticationHostID();
	static bool IsHost();
	static bool IsDedicated();

private:
	static void Init();

private:
	static int32 s_ClientHostID;
	static int32 s_AuthenticationID;
	static bool s_IsHost;
	static bool s_IsDedicated;
};
