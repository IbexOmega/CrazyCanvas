#pragma once

#include "Types.h"

#include "ECS/Entity.h"

namespace LambdaEngine
{
	class IClient;

	class MultiplayerUtilBase
	{
		friend class ClientSystem;
		friend class ServerSystem;

	public:
		DECL_ABSTRACT_CLASS(MultiplayerUtilBase);

		virtual Entity GetEntity(int32 networkUID) const = 0;
		virtual int32 GetNetworkUID(Entity entity) const = 0;
		virtual void RegisterEntity(Entity entity, int32 networkUID) = 0;
		virtual void UnregisterEntity(Entity entity) = 0;
	};
}