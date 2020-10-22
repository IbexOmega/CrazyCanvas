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
		virtual int32 GetNetworkUID(Entity entity) const override final;
		virtual void RegisterEntity(Entity entity, int32 networkUID) override final;

	private:
		ClientUtilsImpl();

	private:
		std::unordered_map<int32, Entity> m_NetworkUIDToEntityMapper;
		std::unordered_map<int32, Entity> m_EntityToNetworkUIDMapper;
	};
}