#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntityRegistry.h"
#include "ECS/System.h"
#include "Utilities/IDGenerator.h"

#include <functional>
#include <map>
#include <typeindex>
#include <unordered_set>

namespace LambdaEngine
{
    class ComponentHandler;
    struct ComponentHandlerRegistration;

    struct EntitySubscription
    {
        // Stores IDs of entities found using subscription
        IDVector* pSubscriber;
        TArray<std::type_index> ComponentTypes;
        // Optional: Called after an entity was added due to the subscription
        std::function<void(Entity)> OnEntityAdded;
        // Optional: Called before an entity was removed
        std::function<void(Entity)> OnEntityRemoved;
    };

    // Indices for subscription storage
    struct SubscriptionStorageIndex
    {
        uint32 SystemID;
        // A system can have multiple subscriptions stored in an array, hence the subscription index
        uint32 SubIdx;
    };

    struct ComponentStorage
    {
        IDContainer* pContainer;
        // Optional: Used when a component needs to be destroyed by its handler. Called alongside erasing the component from its container.
        std::function<void(Entity)> ComponentDestructor;
    };

    class EntityPublisher
    {
    public:
        EntityPublisher(EntityRegistry* pEntityRegistry);
        ~EntityPublisher() = default;

        void RegisterComponentHandler(const ComponentHandlerRegistration& componentHandlerRegistration);
        void DeregisterComponentHandler(ComponentHandler* handler);
        ComponentHandler* GetComponentHandler(const std::type_index& handlerType);

        // Returns a subscription ID
        uint32 SubscribeToComponents(const EntitySubscriberRegistration& subscriberRegistration);
        void UnsubscribeFromComponents(uint32 subscriptionID);

        // Notifies subscribed systems that a new component has been made
        void NewComponent(Entity entityID, std::type_index componentType);
        // Notifies subscribed systems that a component has been deleted
        void RemovedComponent(Entity entityID, std::type_index componentType);

        THashTable<std::type_index, ComponentStorage>& GetComponentStorage() { return m_ComponentStorage; }

    private:
        static void EliminateDuplicateTIDs(TArray<std::type_index>& TIDs);

    private:
        // Map component types to resources used when systems subscribe
        THashTable<std::type_index, ComponentStorage> m_ComponentStorage;
        // Map component types to subscriptions. Deleted only when a subscribing system unsubscribes.
        std::unordered_multimap<std::type_index, SubscriptionStorageIndex> m_ComponentSubscriptions;

        // Map systems' IDs to their subscriptions
        IDDVector<TArray<EntitySubscription>> m_SubscriptionStorage;
        IDGenerator m_SystemIDGenerator;

        THashTable<std::type_index, ComponentHandler*> m_ComponentHandlers;

        const EntityRegistry* m_pEntityRegistry;
    };
}
