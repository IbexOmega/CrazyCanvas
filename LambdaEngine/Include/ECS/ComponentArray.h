#pragma once

#include "Defines.h"
#include "Entity.h"

namespace LambdaEngine
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};

	template<typename Comp>
	class ComponentArray : public IComponentArray
	{
	public:
		ComponentArray();
		virtual ~ComponentArray() = default;

		Comp& Insert(Entity entity, const Comp& comp);
		void Remove(Entity entity);
		
		Comp& GetData(Entity entity);

		void EntityDestroyed(Entity) override final;

	private:
		TArray<Comp> m_Data;
		std::unordered_map<Entity, uint64> m_EntityToIndex;
		std::unordered_map<uint64, Entity> m_IndexToEntity;
		uint64 m_Size = 0;
		uint64 m_Capacity = 64;
	};

	template<typename Comp>
	inline ComponentArray<Comp>::ComponentArray()
	{
		m_Data.Resize(m_Capacity);
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::Insert(Entity entity, const Comp& comp)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) == m_EntityToIndex.end(), "Trying to add a component that already exists!");

		// Resize if more data is added.
		if (m_Size+1 > m_Data.GetSize())
			m_Data.Resize(m_Size + m_Capacity);

		// Get new index and add the component to that position.
		uint64 newIndex = m_Size++;
		m_EntityToIndex[entity] = newIndex;
		m_IndexToEntity[newIndex] = entity;
		memcpy(&m_Data[newIndex], &comp, sizeof(Comp));
		return m_Data[newIndex];
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::Remove(Entity entity)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) != m_EntityToIndex.end(), "Trying to remove a component that does not exist!");

		// Swap the removed component with the last component.
		uint64 currentIndex = m_EntityToIndex[entity];
		uint64 lastIndex = --m_Size;
		m_Data[currentIndex] = m_Data[lastIndex];

		// Update entity-index maps.
		Entity lastEntity = m_IndexToEntity[lastIndex];
		m_EntityToIndex[lastEntity] = currentIndex;
		m_IndexToEntity[currentIndex] = lastEntity;
		
		// Remove the deleted component's entry.
		m_EntityToIndex.erase(entity);
		m_IndexToEntity.erase(lastIndex);
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::GetData(Entity entity)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) != m_EntityToIndex.end(), "Trying to get a component that does not exist!");
		uint64 index = m_EntityToIndex[entity];
		return m_Data[index];
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::EntityDestroyed(Entity entity)
	{
		// Remove component related to the entity if it exists.
		if (m_EntityToIndex.find(entity) != m_EntityToIndex.end())
			Remove(entity);
	}
}