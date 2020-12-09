#include "Game/PlayerIndexHelper.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

namespace LambdaEngine
{
	TArray<Entity>	PlayerIndexHelper::s_Entities;
	uint32 			PlayerIndexHelper::s_MaxPlayers = 10;

	void PlayerIndexHelper::Reset()
	{
		s_Entities.Clear();
	}

	uint32 PlayerIndexHelper::GetPlayerIndex(Entity entity)
	{
		// Check if already added
		for (uint32 i = 0; i < s_Entities.GetSize(); i++)
		{
			if (s_Entities[i] == entity)
			{
				return i;
			}
		}

		// Check if player
		ECSCore* pECS = ECSCore::GetInstance();
		PlayerBaseComponent comp;
		if(pECS->GetConstComponentIf<PlayerBaseComponent>(entity, comp))
		{
			if (s_Entities.GetSize() < s_MaxPlayers)
			{
				uint32 index = s_Entities.GetSize();
				s_Entities.PushBack(entity);
				return index;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Requested an index of entity %d, but entity does not have a mapped index! Array size: %d, max players: %d",
			entity, s_Entities.GetSize(), s_MaxPlayers);

		return UINT32_MAX;
	}

	Entity PlayerIndexHelper::GetPlayerEntity(uint32 index)
	{
		if (index < s_Entities.GetSize())
		{
			return s_Entities[index];
		}

		LOG_WARNING("[PlayerIndexHelper]: Requested player entity of index %d, but index not valid! Size of array: %d",
			index, s_Entities.GetSize());

		return (Entity)UINT32_MAX;
	}

	bool PlayerIndexHelper::AddPlayerEntity(Entity entity)
	{
		// Check if player
		ECSCore* pECS = ECSCore::GetInstance();
		PlayerBaseComponent comp;
		if(pECS->GetConstComponentIf<PlayerBaseComponent>(entity, comp))
		{
			if (s_Entities.GetSize() < s_MaxPlayers)
			{
				s_Entities.PushBack(entity);
				return true;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Tried to add entity %d but failed. Either player was not a player or the array of indices is full. Array size: %d, max: %d",
			entity, s_Entities.GetSize(), s_MaxPlayers);

		return false;
	}

	void PlayerIndexHelper::RemovePlayerEntity(Entity entity)
	{
		for (uint32 i = 0; i < s_Entities.GetSize(); i++)
		{
			if (s_Entities[i] == entity)
			{
				s_Entities[i] = s_Entities[s_Entities.GetSize() - 1];
				s_Entities.PopBack();
				return;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Tried to remove an entity does not exist in the previously added entites! Entity: %d, Array size: %d",
			entity, s_Entities.GetSize());
	}

	bool PlayerIndexHelper::IsEntityValid(Entity entity)
	{
		auto it = std::find(s_Entities.Begin(), s_Entities.End(), entity);
		return it != s_Entities.End();
	}
}