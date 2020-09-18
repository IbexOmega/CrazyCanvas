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

		virtual void Remove(Entity entity) = 0;

		virtual const TArray<uint32>& GetIDs() const = 0;

		virtual bool HasComponent(Entity entity) const = 0;
	};

	template<typename Comp>
	class LAMBDA_API ComponentArray : public IComponentArray
	{
	public:
		ComponentArray();
		virtual ~ComponentArray() = default;

		void EntityDestroyed(Entity) override final;

		Comp& Insert(Entity entity, const Comp& comp);
		void Remove(Entity entity) override final;

		Comp& GetData(Entity entity);
		const TArray<uint32>& GetIDs() const override final { return m_IDs; }

		bool HasComponent(Entity entity) const override final { return m_EntityToIndex.find(entity) != m_EntityToIndex.end(); }

	private:
		TArray<Comp> m_Data;
		TArray<uint32> m_IDs;
		std::unordered_map<Entity, uint32> m_EntityToIndex;
		uint32 m_Size = 0;
		uint32 m_Capacity = 64;
	};

	template<typename Comp>
	inline ComponentArray<Comp>::ComponentArray()
	{
		m_Data.Resize(m_Capacity);
		m_IDs.Resize(m_Capacity);
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::Insert(Entity entity, const Comp& comp)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) == m_EntityToIndex.end(), "Trying to add a component that already exists!");

		// Resize if more data is added.
		if (m_Size+1 > m_Data.GetSize())
		{
			m_Data.Resize(m_Size + m_Capacity);
			m_IDs.Resize(m_Size + m_Capacity);
		}

		// Get new index and add the component to that position.
		uint32 newIndex = m_Size++;
		m_EntityToIndex[entity] = newIndex;
		m_Data[newIndex] = comp;
		m_IDs[newIndex] = entity;
		return m_Data[newIndex];
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::Remove(Entity entity)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) != m_EntityToIndex.end(), "Trying to remove a component that does not exist!");

		// Swap the removed component with the last component.
		uint32 currentIndex = m_EntityToIndex[entity];
		uint32 lastIndex = --m_Size;
		m_Data[currentIndex] = m_Data[lastIndex];
		m_IDs[currentIndex] = m_IDs[lastIndex];

		// Update entity-index maps.
		m_EntityToIndex[m_IDs[lastIndex]] = currentIndex;

		// Remove the deleted component's entry.
		m_EntityToIndex.erase(entity);
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::GetData(Entity entity)
	{
		VALIDATE_MSG(m_EntityToIndex.find(entity) != m_EntityToIndex.end(), "Trying to get a component that does not exist!");
		uint32 index = m_EntityToIndex[entity];
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
