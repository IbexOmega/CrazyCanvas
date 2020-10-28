#include "Multiplayer/ServerHostHelper.h"


int32 ServerHostHelper::m_sServerHostID = -1;
int32 ServerHostHelper::m_sClientHostID = -1;

void ServerHostHelper::SetServerHostID(int32 serverHostID)
{
	m_sServerHostID = serverHostID;
}

void ServerHostHelper::SetClientHostID(int32 clientHostID)
{
	m_sClientHostID = clientHostID;
}

int32 ServerHostHelper::GetServerHostID()
{
	return m_sServerHostID;
}

int32 ServerHostHelper::GetClientHostID()
{
	return m_sClientHostID;
}
