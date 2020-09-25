#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "Threading/API/SpinLock.h"
#include "Utilities/IDGenerator.h"

#include <stack>
#include <typeindex>
#include <unordered_set>

namespace LambdaEngine
{
    class ComponentType;

    // Map Entities to the set of component types they are registered to
    typedef IDDVector<std::unordered_set<const ComponentType*>> EntityRegistryPage;

    class EntityRegistry
    {
    public:
        EntityRegistry();
        ~EntityRegistry() = default;

        void RegisterComponentType(Entity entity, const ComponentType* pComponentType);
        void DeregisterComponentType(Entity entity, const ComponentType* pComponentType);

        // Queries whether or not the entity has all the specified types
        bool EntityHasAllowedTypes(Entity entity, const TArray<const ComponentType*>& queryTypes, const TArray<const ComponentType*>& excludedComponentTypes) const;

        Entity CreateEntity();
        void DeregisterEntity(Entity entity);

        void AddPage();
        void RemovePage();
        const EntityRegistryPage& GetTopRegistryPage() const { return m_EntityPages.top(); }

    private:
        std::stack<EntityRegistryPage> m_EntityPages;
        IDGenerator m_EntityIDGen;
        mutable SpinLock m_Lock;
    };
}
