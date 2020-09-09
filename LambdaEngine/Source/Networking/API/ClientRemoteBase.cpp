#include "Networking/API/ClientRemoteBase.h"

namespace LambdaEngine
{
    ClientRemoteBase::ClientRemoteBase() : 
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_Release(false),
		m_DisconnectedByRemote(false)
    {

    }
}