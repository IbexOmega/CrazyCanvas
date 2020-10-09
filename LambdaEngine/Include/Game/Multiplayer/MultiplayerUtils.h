#pragma once

#include "Types.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/PacketFunction.h"

namespace LambdaEngine
{
	class MultiplayerUtilBase;

	class MultiplayerUtils
	{
		friend class ClientSystem;
		friend class ServerSystem;
		friend class ClientRemoteSystem;

	public:
		DECL_STATIC_CLASS(MultiplayerUtils);

		static Entity GetEntity(int32 networkUID);
		static void SubscribeToPacketType(uint16 packetType, const PacketFunction& func);

	private:
		static void Init(bool server);
		static void Release();

	private:
		static MultiplayerUtilBase* s_pMultiplayerUtility;
	};
}