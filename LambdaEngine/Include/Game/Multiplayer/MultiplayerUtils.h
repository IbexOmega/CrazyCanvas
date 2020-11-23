#pragma once

#include "Types.h"

#include "ECS/Entity.h"

namespace LambdaEngine
{
	class MultiplayerUtilBase;
	class IClient;

	class MultiplayerUtils
	{
		friend class ClientSystem;
		friend class ServerSystem;
		friend class NetworkSystem;

	public:
		DECL_STATIC_CLASS(MultiplayerUtils);

		static bool IsServer();
		static Entity GetEntity(int32 networkUID);
		static int32 GetNetworkUID(Entity entity);
		static bool IsSingleplayer();
		static bool HasWriteAccessToEntity(Entity entity);

		static void SetIsSingleplayer(bool value);

	private:
		static void Init(bool server);
		static void Release();

		static void RegisterEntity(Entity entity, int32 networkUID);
		static void UnregisterEntity(Entity entity);

	private:
		static MultiplayerUtilBase* s_pMultiplayerUtility;
		static bool s_IsServer;
		static bool s_IsSinglePlayer;
	};
}