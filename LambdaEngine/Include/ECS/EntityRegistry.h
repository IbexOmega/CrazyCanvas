#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "Utilities/IDGenerator.h"

#include <stack>
#include <typeindex>
#include <unordered_set>

namespace LambdaEngine
{
    // Map Entities to the set of component types they are registered to
    typedef IDDVector<std::unordered_set<std::type_index>> EntityRegistryPage;

    class EntityRegistry
    {
    public:
        EntityRegistry();
        ~EntityRegistry();

        void RegisterComponentType(Entity entity, std::type_index componentType);
        void DeregisterComponentType(Entity entity, std::type_index componentType);

        // Queries whether or not the entity has all the specified types
        bool EntityHasTypes(Entity entity, const TArray<std::type_index>& queryTypes) const;

        Entity CreateEntity();
        void DeregisterEntity(Entity entity);

        void AddPage();
        void RemovePage();
        const EntityRegistryPage& GetTopRegistryPage() const { return m_EntityPages.top(); }

    private:
        std::stack<EntityRegistryPage> m_EntityPages;
        IDGenerator m_EntityIDGen;
    };
}
