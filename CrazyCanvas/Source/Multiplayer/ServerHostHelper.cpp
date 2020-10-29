#include "Multiplayer/ServerHostHelper.h"


int32 ServerHostHelper::m_sClientHostID = -1;
int32 ServerHostHelper::m_sAuthenticationID = -1;

void ServerHostHelper::SetClientHostID(int32 serverHostID)
{
	m_sClientHostID = serverHostID;
}

void ServerHostHelper::SetAuthenticationID(int32 clientHostID)
{
	m_sAuthenticationID = clientHostID;
}

int32 ServerHostHelper::GetClientHostID()
{
	return m_sClientHostID;
}

int32 ServerHostHelper::GetAuthenticationHostID()
{
	return m_sAuthenticationID;
}
