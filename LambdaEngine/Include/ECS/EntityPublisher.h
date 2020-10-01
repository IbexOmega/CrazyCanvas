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
    class ComponentStorage;
    struct ComponentHandlerRegistration;

    struct EntitySubscription
    {
        // Stores IDs of entities found using subscription
        IDVector* pSubscriber;
        TArray<const ComponentType*> ComponentTypes;
        TArray<const ComponentType*> ExcludedComponentTypes;
        // Optional: Called after an entity was added due to the subscription
        std::function<void(Entity)> OnEntityAdded;
        // Optional: Called before an entity was removed
        std::function<void(Entity)> OnEntityRemoval;
    };

    // Indices for subscription storage
    struct SubscriptionStorageIndex
    {
        uint32 SystemID;
        // A system can have multiple subscriptions stored in an array, hence the subscription index
        uint32 SubIdx;
    };

    class EntityPublisher
    {
    public:
        EntityPublisher(const ComponentStorage* pComponentStorage, const EntityRegistry* pEntityRegistry);
        ~EntityPublisher() = default;

        // Returns a subscription ID
        uint32 SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration);
        void UnsubscribeFromEntities(uint32 subscriptionID);

        // Notifies subscribers that a component has been added
        void PublishComponent(Entity entityID, const ComponentType* pComponentType);
        // Notifies subscribers that a component has been deleted
        void UnpublishComponent(Entity entityID, const ComponentType* pComponentType);

    private:
        static void EliminateDuplicateTIDs(TArray<const ComponentType*>& TIDs);

    private:
        // Map component types to subscriptions. Deleted only when a subscribing system unsubscribes.
        std::unordered_multimap<const ComponentType*, SubscriptionStorageIndex> m_ComponentSubscriptions;

        // Map systems' IDs to their subscriptions
        IDDVector<TArray<EntitySubscription>> m_SubscriptionStorage;
        IDGenerator m_SystemIDGenerator;

        THashTable<const ComponentType*, ComponentHandler*> m_ComponentHandlers;

        const ComponentStorage* m_pComponentStorage;
        const EntityRegistry* m_pEntityRegistry;
    };
}
