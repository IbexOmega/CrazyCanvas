#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/Multiplayer/MultiplayerUtilBase.h"
#include "Game/Multiplayer/ServerUtilsImpl.h"
#include "Game/Multiplayer/ClientUtilsImpl.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkStatistics.h"

namespace LambdaEngine
{
	MultiplayerUtilBase* MultiplayerUtils::s_pMultiplayerUtility = nullptr;
	bool MultiplayerUtils::s_IsServer = false;
	IClientEntityAccessor* MultiplayerUtils::s_pClientEntityAccessor = nullptr;

	bool MultiplayerUtils::IsServer()
	{
		return s_IsServer;
	}

	Entity MultiplayerUtils::GetEntity(int32 networkUID)
	{
		return s_pMultiplayerUtility->GetEntity(networkUID);
	}

	void MultiplayerUtils::RegisterEntity(Entity entity, int32 networkUID)
	{
		s_pMultiplayerUtility->RegisterEntity(entity, networkUID);
	}

	Entity MultiplayerUtils::GetEntityPlayer(IClient* pClient)
	{
		return s_pClientEntityAccessor->GetEntityPlayer(s_pMultiplayerUtility->GetSaltAsUID(pClient));
	}

	void MultiplayerUtils::RegisterClientEntityAccessor(IClientEntityAccessor* pAccessor)
	{
		s_pClientEntityAccessor = pAccessor;
	}

	void MultiplayerUtils::Init(bool server)
	{
		Release();
		s_IsServer = server;

		if (server)
			s_pMultiplayerUtility = DBG_NEW ServerUtilsImpl();
		else
			s_pMultiplayerUtility = DBG_NEW ClientUtilsImpl();
	}

	void MultiplayerUtils::Release()
	{
		SAFEDELETE(s_pMultiplayerUtility);
	}
}