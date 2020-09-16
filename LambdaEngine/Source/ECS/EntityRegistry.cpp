#include "ECS/EntityRegistry.h"

#include "Log/Log.h"

namespace LambdaEngine
{
    EntityRegistry::EntityRegistry()
    {
        AddPage();
    }

    void EntityRegistry::RegisterComponentType(Entity entity, std::type_index componentType)
    {
        EntityRegistryPage& topPage = m_EntityPages.top();

        if (!topPage.HasElement(entity)) {
            // Initialize a new set
            topPage.PushBack({componentType}, entity);
        } else {
            // Add the component type to the set
            topPage.IndexID(entity).insert(componentType);
        }
    }

    void EntityRegistry::DeregisterComponentType(Entity entity, std::type_index componentType)
    {
        EntityRegistryPage& topPage = m_EntityPages.top();
        if (!topPage.HasElement(entity)) {
            LOG_WARNING("Attempted to deregister a component type (%s) from an unregistered entity: %ld", componentType.name(), entity);
        } else {
            topPage.IndexID(entity).erase(componentType);
        }
    }

    bool EntityRegistry::EntityHasTypes(Entity entity, const TArray<std::type_index>& queryTypes) const
    {
        const EntityRegistryPage& topPage = m_EntityPages.top();
        const std::unordered_set<std::type_index>& entityTypes = topPage.IndexID(entity);

        for (const std::type_index& type : queryTypes) {
            auto got = entityTypes.find(type);
            if (got == entityTypes.end()) {
                return false;
            }
        }

        return true;
    }

    Entity EntityRegistry::CreateEntity()
    {
        Entity newEntity = m_EntityIDGen.GenID();

        EntityRegistryPage& topPage = m_EntityPages.top();
        topPage.PushBack({}, newEntity);

        return newEntity;
    }

    void EntityRegistry::DeregisterEntity(Entity entity)
    {
        EntityRegistryPage& topPage = m_EntityPages.top();
        if (!topPage.HasElement(entity)) {
            LOG_WARNING("Attempted to deregister an unregistered entity: %ld", entity);
            return;
        }

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
        for (Entity entity : entities) {
            m_EntityIDGen.PopID(entity);
        }

        m_EntityPages.pop();
    }
}
