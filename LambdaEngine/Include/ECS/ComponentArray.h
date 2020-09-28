#pragma once

#include "Containers/THashTable.h"
#include "Defines.h"
#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	class ComponentStorage;

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;

		virtual void UnsetComponentOwner() = 0;

		virtual const TArray<uint32>& GetIDs() const = 0;

		virtual bool HasComponent(Entity entity) const = 0;

	protected:
		// Systems or other external users should not be able to perform immediate deletions
		friend ComponentStorage;
		virtual void Remove(Entity entity) = 0;
	};

	template<typename Comp>
	class LAMBDA_API ComponentArray : public IComponentArray
	{
	public:
		ComponentArray() = default;
		virtual ~ComponentArray() = default;

		void SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership) { m_ComponentOwnership = componentOwnership };
		void UnsetComponentOwner() override final { m_ComponentOwnership = {}; };

		Comp& Insert(Entity entity, const Comp& comp);

		Comp& GetData(Entity entity);
		const Comp& GetData(Entity entity) const;
		const TArray<uint32>& GetIDs() const override final { return m_IDs; }

		bool HasComponent(Entity entity) const override final { return m_EntityToIndex.find(entity) != m_EntityToIndex.end(); }

	protected:
		void Remove(Entity entity) override final;

	private:
		TArray<Comp> m_Data;
		TArray<uint32> m_IDs;
		THashTable<Entity, uint32> m_EntityToIndex;

		ComponentOwnership<Comp> m_ComponentOwnership;
	};

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::Insert(Entity entity, const Comp& comp)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr == m_EntityToIndex.end(), "Trying to add a component that already exists!");

		// Get new index and add the component to that position.
		uint32 newIndex = m_Data.GetSize();
		m_EntityToIndex[entity] = newIndex;
		m_IDs.PushBack(entity);
		return m_Data.PushBack(comp);
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::GetData(Entity entity)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to get a component that does not exist!");

		Comp& component = m_Data[indexItr->second];

		if constexpr (Comp::HasDirtyFlag())
			component.Dirty = true;

		return component;
	}

	template<typename Comp>
	inline const Comp& ComponentArray<Comp>::GetData(Entity entity) const
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to get a component that does not exist!");

		return m_Data[indexItr->second];
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::Remove(Entity entity)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to remove a component that does not exist!");

		uint32 currentIndex = indexItr->second;

		if (m_ComponentOwnership.Destructor)
			m_ComponentOwnership.Destructor(m_Data[currentIndex]);

		// Swap the removed component with the last component.
		m_Data[currentIndex] = m_Data.GetBack();
		m_IDs[currentIndex] = m_IDs.GetBack();

		// Update entity-index maps.
		m_EntityToIndex[m_IDs.GetBack()] = currentIndex;

		m_Data.PopBack();
		m_IDs.PopBack();

		// Remove the deleted component's entry.
		m_EntityToIndex.erase(indexItr);
	}
}
