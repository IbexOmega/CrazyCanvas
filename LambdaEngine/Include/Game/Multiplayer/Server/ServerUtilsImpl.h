#pragma once

#include "Game/Multiplayer/MultiplayerUtilBase.h"

namespace LambdaEngine
{
	class ServerUtilsImpl : public MultiplayerUtilBase
	{
		friend class MultiplayerUtils;

	public:
		DECL_UNIQUE_CLASS(ServerUtilsImpl);
		virtual ~ServerUtilsImpl();

		virtual Entity GetEntity(int32 networkUID) const override final;
		virtual int32 GetNetworkUID(Entity entity) const override final;
		virtual void RegisterEntity(Entity entity, int32 networkUID) override final;
		virtual void UnregisterEntity(Entity entity) override final;

	private:
		ServerUtilsImpl();
	};
}