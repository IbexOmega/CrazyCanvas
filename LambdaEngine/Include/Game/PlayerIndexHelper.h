#pragma once

#include "ECS/Entity.h"

namespace LambdaEngine
{
	class PlayerIndexHelper
	{
	public:
		DECL_STATIC_CLASS(PlayerIndexHelper);

		/*
		*	Converts the entity ID to a player index to be used for indexing in lists.
		*	Creates an index if the entity has not been previously added.
		*	Return: Returns a valid index of max size player count. If entity was not an entity, UINT32_MAX will be returned.
		*/
		static uint32 GetPlayerIndex(Entity entity);

		/*
		*	Converts the index to a previously added entity
		*	Return: Returns a valid entity if the entity has been added previously.
		*			If entity hasn't been added before, returns UINT32_MAX
		*/
		static Entity GetPlayerEntity(uint32 index);

		/*
		*	Add a new player entity
		*/
		static bool AddPlayerEntity(Entity entity);

		/*
		*	Removes a previously added player entity
		*	NOTE: This will change the indexing order of the entites to have it
		*			be a compact array. Use GetPlayerIndex to make sure to always
		*			have the correct index.
		*/
		static void RemovePlayerEntity(Entity entity);

		/*
		*	Gets how many entities have a mapped index
		*/
		static uint32 GetNumOfIndices() { return s_Entities.GetSize(); };

		/*
		*	Returns true if the given entity has a mapped index
		*/
		static bool IsEntityValid(Entity entity);

	private:
		static TArray<Entity> s_Entities;
		static uint32 s_MaxPlayers;
	};
}