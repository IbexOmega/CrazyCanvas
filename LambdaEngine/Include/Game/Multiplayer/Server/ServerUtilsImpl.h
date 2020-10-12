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
		virtual void RegisterEntity(Entity entity, int32 networkUID) override final;
		virtual uint64 GetSaltAsUID(IClient* pClient) override final;

	private:
		ServerUtilsImpl();
	};
}