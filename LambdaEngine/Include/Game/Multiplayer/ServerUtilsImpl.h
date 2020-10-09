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

	private:
		ServerUtilsImpl();
	};
}