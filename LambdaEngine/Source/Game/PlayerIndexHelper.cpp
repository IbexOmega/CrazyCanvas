#include "Game/PlayerIndexHelper.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

namespace LambdaEngine
{
	TArray<Entity>	PlayerIndexHelper::m_Entities;
	uint32 			PlayerIndexHelper::m_MaxPlayers = 10;

	uint32 PlayerIndexHelper::GetPlayerIndex(Entity entity)
	{
		// Check if already added
		for (uint32 i = 0; i < m_Entities.GetSize(); i++)
		{
			if (m_Entities[i] == entity)
			{
				return i;
			}
		}

		// Check if player
		ECSCore* pECS = ECSCore::GetInstance();
		PlayerBaseComponent comp;
		if(pECS->GetConstComponentIf<PlayerBaseComponent>(entity, comp))
		{
			if (m_Entities.GetSize() < m_MaxPlayers)
			{
				uint32 index = m_Entities.GetSize();
				m_Entities.PushBack(entity);
				return index;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Requested an index of entity %d, but entites is not added previously and cannot be added now! Array size: %d, max players: %d",
			entity, m_Entities.GetSize(), m_MaxPlayers);

		return UINT32_MAX;
	}

	Entity PlayerIndexHelper::GetPlayerEntity(uint32 index)
	{
		if (index < m_Entities.GetSize())
		{
			return m_Entities[index];
		}

		LOG_WARNING("[PlayerIndexHelper]: Requested player entity of index %d, but index not valid! Size of array: %d",
			index, m_Entities.GetSize());

		return (Entity)UINT32_MAX;
	}

	bool PlayerIndexHelper::AddPlayerEntity(Entity entity)
	{
		// Check if player
		ECSCore* pECS = ECSCore::GetInstance();
		PlayerBaseComponent comp;
		if(pECS->GetConstComponentIf<PlayerBaseComponent>(entity, comp))
		{
			if (m_Entities.GetSize() < m_MaxPlayers)
			{
				m_Entities.PushBack(entity);
				return true;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Tried to add entity %d but failed. Either player was not a player or the array of indices is full. Array size: %d, max: %d",
			entity, m_Entities.GetSize(), m_MaxPlayers);

		return false;
	}

	void PlayerIndexHelper::RemovePlayerEntity(Entity entity)
	{
		for (uint32 i = 0; i < m_Entities.GetSize(); i++)
		{
			if (m_Entities[i] == entity)
			{
				m_Entities[i] = m_Entities[m_Entities.GetSize() - 1];
				m_Entities.PopBack();
				return;
			}
		}

		LOG_WARNING("[PlayerIndexHelper]: Tried to remove an entity does not exist in the previously added entites! Entity: %d, Array size: %d",
			entity, m_Entities.GetSize());
	}
}