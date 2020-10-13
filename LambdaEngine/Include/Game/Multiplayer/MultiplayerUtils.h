#pragma once

#include "Types.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/PacketFunction.h"

namespace LambdaEngine
{
	class MultiplayerUtilBase;
	class IClient;

	class IClientEntityAccessor
	{
	public:
		DECL_INTERFACE(IClientEntityAccessor);

		virtual Entity GetEntityPlayer(uint64 saltUID) = 0;
	};

	class MultiplayerUtils
	{
		friend class ClientSystem;
		friend class ServerSystem;
		friend class ClientRemoteSystem;

	public:
		DECL_STATIC_CLASS(MultiplayerUtils);

		static bool IsServer();
		static Entity GetEntity(int32 networkUID);
		static void RegisterEntity(Entity entity, int32 networkUID);
		static Entity GetEntityPlayer(IClient* pClient);
		static void RegisterClientEntityAccessor(IClientEntityAccessor* pAccessor);
		static bool IsSingleplayer();

	private:
		static void Init(bool server);
		static void Release();

	private:
		static MultiplayerUtilBase* s_pMultiplayerUtility;
		static bool s_IsServer;
		static bool s_IsSinglePlayer;
		static IClientEntityAccessor* s_pClientEntityAccessor;
	};
}