#include "ECS/EntityRegistry.h"

#include "ECS/ComponentType.h"

#include "Log/Log.h"
#include <mutex>

namespace LambdaEngine
{
	EntityRegistry::EntityRegistry()
	{
		AddPage();
	}

	void EntityRegistry::RegisterComponentType(Entity entity, const ComponentType* pComponentType)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		EntityRegistryPage& topPage = m_EntityPages.top();
		if (!topPage.HasElement(entity))
		{
			// Initialize a new set
			topPage.PushBack({ pComponentType }, entity);
		}
		else
		{
			// Add the component type to the set
			topPage.IndexID(entity).insert(pComponentType);
		}
	}

	void EntityRegistry::DeregisterComponentType(Entity entity, const ComponentType* pComponentType)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		EntityRegistryPage& topPage = m_EntityPages.top();
		if (!topPage.HasElement(entity))
		{
			LOG_WARNING("Attempted to deregister a component type (%s) from an unregistered entity: %ld", pComponentType->GetName(), entity);
		}
		else
		{
			topPage.IndexID(entity).erase(pComponentType);
		}
	}

	bool EntityRegistry::EntityHasAllowedTypes(Entity entity, const TArray<const ComponentType*>& allowedTypes, const TArray<const ComponentType*>& disallowedTypes) const
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		const EntityRegistryPage& topPage = m_EntityPages.top();
		const std::unordered_set<const ComponentType*>& entityTypes = topPage.IndexID(entity);

		const bool hasDisallowedType = std::any_of(disallowedTypes.Begin(), disallowedTypes.End(), [entityTypes](const ComponentType* pType) {
			return entityTypes.contains(pType);
		});

		if (hasDisallowedType)
		{
			return false;
		}

		return std::none_of(allowedTypes.Begin(), allowedTypes.End(), [entityTypes](const ComponentType* pType) {
			return !entityTypes.contains(pType);
		});
	}

	bool EntityRegistry::EntityHasAllTypes(Entity entity, const TArray<const ComponentType*>& types) const
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		const EntityRegistryPage& topPage = m_EntityPages.top();
		const std::unordered_set<const ComponentType*>& entityTypes = topPage.IndexID(entity);

		// Check that none of the entity types are missing
		return std::none_of(types.Begin(), types.End(), [entityTypes](const ComponentType* pType) {
			return !entityTypes.contains(pType);
		});
	}

	bool EntityRegistry::EntityHasAnyOfTypes(Entity entity, const TArray<const ComponentType*>& types) const
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		const EntityRegistryPage& topPage = m_EntityPages.top();
		const std::unordered_set<const ComponentType*>& entityTypes = topPage.IndexID(entity);

		return std::any_of(types.Begin(), types.End(), [entityTypes](const ComponentType* pType) {
			return entityTypes.contains(pType);
		});
	}

	Entity EntityRegistry::CreateEntity()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		const Entity newEntity = m_EntityIDGen.GenID();

		EntityRegistryPage& topPage = m_EntityPages.top();
		topPage.PushBack({}, newEntity);

		return newEntity;
	}

	void EntityRegistry::DeregisterEntity(Entity entity)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		EntityRegistryPage& topPage = m_EntityPages.top();
		if (!topPage.HasElement(entity))
			return;

		topPage.Pop(entity);
		m_EntityIDGen.PopID(entity);
	}

	void EntityRegistry::AddPage()
	{
		m_EntityPages.push({});
	}

	void EntityRegistry::RemovePage()
	{
		EntityRegistryPage& topPage = m_EntityPages.top();
		const TArray<Entity> entities = topPage.GetIDs();
		for (Entity entity : entities)
		{
			m_EntityIDGen.PopID(entity);
		}

		m_EntityPages.pop();
	}
}
