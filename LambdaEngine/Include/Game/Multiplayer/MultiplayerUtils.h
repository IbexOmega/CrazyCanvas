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
		friend class ClientRemoteSystem;

	public:
		DECL_STATIC_CLASS(MultiplayerUtils);

		static bool IsServer();
		static Entity GetEntity(int32 networkUID);
		static int32 GetNetworkUID(Entity entity);
		static void RegisterEntity(Entity entity, int32 networkUID);
		static bool IsSingleplayer();
		static bool HasWriteAccessToEntity(Entity entity);

	private:
		static void Init(bool server);
		static void Release();

	private:
		static MultiplayerUtilBase* s_pMultiplayerUtility;
		static bool s_IsServer;
		static bool s_IsSinglePlayer;
	};
}