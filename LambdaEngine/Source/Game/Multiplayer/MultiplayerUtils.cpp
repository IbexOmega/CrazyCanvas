#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/Multiplayer/MultiplayerUtilBase.h"
#include "Game/Multiplayer/ServerUtilsImpl.h"
#include "Game/Multiplayer/ClientUtilsImpl.h"

namespace LambdaEngine
{
	MultiplayerUtilBase* MultiplayerUtils::s_pMultiplayerUtility = nullptr;

	Entity MultiplayerUtils::GetEntity(int32 networkUID)
	{
		return s_pMultiplayerUtility->GetEntity(networkUID);
	}

	void MultiplayerUtils::SubscribeToPacketType(uint16 packetType, const PacketFunction& func)
	{
		s_pMultiplayerUtility->SubscribeToPacketType(packetType, func);
	}

	void MultiplayerUtils::Init(bool server)
	{
		Release();

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