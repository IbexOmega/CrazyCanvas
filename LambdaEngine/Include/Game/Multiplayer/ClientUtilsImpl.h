#pragma once

#include "Game/Multiplayer/MultiplayerUtilBase.h"

namespace LambdaEngine
{
	class ClientUtilsImpl : public MultiplayerUtilBase
	{
		friend class MultiplayerUtils;
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(ClientUtilsImpl);
		virtual ~ClientUtilsImpl();

		virtual Entity GetEntity(int32 networkUID) const override final;

	private:
		ClientUtilsImpl();

		void RegisterEntity(Entity entity, int32 networkUID);

	private:
		std::unordered_map<int32, Entity> m_NetworkUIDToEntityMapper;
	};
}