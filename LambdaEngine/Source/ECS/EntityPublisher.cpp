#include "ECS/EntityPublisher.h"

#include "ECS/ComponentStorage.h"
#include "ECS/System.h"
#include "Log/Log.h"

namespace LambdaEngine
{
    EntityPublisher::EntityPublisher(const ComponentStorage* pComponentStorage, const EntityRegistry* pEntityRegistry)
        :m_pComponentStorage(pComponentStorage),
        m_pEntityRegistry(pEntityRegistry)
    {}

    uint32 EntityPublisher::SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration)
    {
        // Create subscriptions from the subscription requests by finding the desired component containers
        TArray<EntitySubscription> subscriptions;
        const TArray<EntitySubscriptionRegistration>& subscriptionRequests = subscriberRegistration.EntitySubscriptionRegistrations;
        subscriptions.Reserve(subscriptionRequests.GetSize());

        // Convert subscription requests to subscriptions. Also eliminate duplicate TIDs in subscription requests.
        for (const EntitySubscriptionRegistration& subReq : subscriptionRequests)
        {
            const TArray<ComponentAccess>& componentRegs = subReq.ComponentAccesses;

            EntitySubscription newSub;
            newSub.ComponentTypes.Reserve(componentRegs.GetSize());

            newSub.pSubscriber = subReq.pSubscriber;
            newSub.OnEntityAdded = subReq.OnEntityAdded;
            newSub.OnEntityRemoved = subReq.OnEntityRemoved;

            newSub.ComponentTypes.Reserve(componentRegs.GetSize());
            for (const ComponentAccess& componentReg : componentRegs)
                newSub.ComponentTypes.PushBack(componentReg.TID);

            EliminateDuplicateTIDs(newSub.ComponentTypes);
            newSub.ComponentTypes.ShrinkToFit();
            subscriptions.EmplaceBack(newSub);
        }

        uint32 subID = m_SystemIDGenerator.GenID();
        m_SubscriptionStorage.PushBack(subscriptions, subID);

        // Map each component type to its subscriptions
        const TArray<EntitySubscription>& subs = m_SubscriptionStorage.IndexID(subID);

        for (uint32 subscriptionNr = 0; subscriptionNr < subs.GetSize(); subscriptionNr += 1)
        {
            const TArray<const ComponentType*>& componentTypes = subs[subscriptionNr].ComponentTypes;

            for (const ComponentType* pComponentType : componentTypes)
                m_ComponentSubscriptions.insert({ pComponentType, {subID, subscriptionNr}});
        }

        // A subscription has been made, notify the system of all existing components it subscribed to
        for (EntitySubscription& subscription : subscriptions)
        {
            const ComponentType* pFirstComponentType = subscription.ComponentTypes.GetFront();
            if (!m_pComponentStorage->HasType(pFirstComponentType))
                continue;

            // Fetch the component vector of the first subscribed component type
            const IComponentArray* pComponentArray = m_pComponentStorage->GetComponentArray(pFirstComponentType);
            const TArray<Entity>& entities = pComponentArray->GetIDs();

            // See which entities in the entity vector also have all the other component types. Register those entities in the system.
            for (Entity entity : entities)
            {
                bool registerEntity = m_pEntityRegistry->EntityHasTypes(entity, subscription.ComponentTypes);

                if (registerEntity)
                {
                    subscription.pSubscriber->PushBack(entity);

                    if (subscription.OnEntityAdded != nullptr)
                    {
                        subscription.OnEntityAdded(entity);
                    }
                }
            }
        }

        return subID;
    }

    void EntityPublisher::UnsubscribeFromEntities(uint32 subscriptionID)
    {
        if (m_SubscriptionStorage.HasElement(subscriptionID) == false) {
            LOG_WARNING("Attempted to deregistered an unregistered system, ID: %d", subscriptionID);
            return;
        }

        // Use the subscriptions to find and delete component subscriptions
        const TArray<EntitySubscription>& subscriptions = m_SubscriptionStorage.IndexID(subscriptionID);

        for (const EntitySubscription& subscription : subscriptions)
        {
            const TArray<const ComponentType*>& componentTypes = subscription.ComponentTypes;

            for (const ComponentType* pComponentType : componentTypes)
            {
                auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

                if (subBucketItr == m_ComponentSubscriptions.end())
                {
                    LOG_WARNING("Attempted to delete non-existent component subscription");
                    // The other component subscriptions might exist, so don't return
                    continue;
                }

                // Find the subscription and delete it
                while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
                {
                    if (subBucketItr->second.SystemID == subscriptionID)
                    {
                        m_ComponentSubscriptions.erase(subBucketItr);
                        break;
                    }

                    subBucketItr++;
                }
            }
        }

        // All component->subscription mappings have been deleted
        m_SubscriptionStorage.Pop(subscriptionID);

        // Recycle system iD
        m_SystemIDGenerator.PopID(subscriptionID);
    }

    void EntityPublisher::PublishComponent(Entity entityID, const ComponentType* pComponentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_map bucket
        auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
        {
            // Use indices stored in the component type -> component storage mapping to get the component subscription
            EntitySubscription& sysSub = m_SubscriptionStorage.IndexID(subBucketItr->second.SystemID)[subBucketItr->second.SubIdx];

            if (m_pEntityRegistry->EntityHasTypes(entityID, sysSub.ComponentTypes))
            {
                sysSub.pSubscriber->PushBack(entityID);

                if (sysSub.OnEntityAdded)
                {
                    sysSub.OnEntityAdded(entityID);
                }
            }

            subBucketItr++;
        }
    }

    void EntityPublisher::UnpublishComponent(Entity entityID, const ComponentType* pComponentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_map bucket
        auto subBucketItr = m_ComponentSubscriptions.find(pComponentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == pComponentType)
        {
            // Use indices stored in the component type -> component storage mapping to get the component subscription
            EntitySubscription& sysSub = m_SubscriptionStorage.IndexID(subBucketItr->second.SystemID)[subBucketItr->second.SubIdx];

            if (!sysSub.pSubscriber->HasElement(entityID))
            {
                subBucketItr++;
                continue;
            }

            if (sysSub.OnEntityRemoved)
            {
                sysSub.OnEntityRemoved(entityID);
            }

            sysSub.pSubscriber->Pop(entityID);

            subBucketItr++;
        }
    }

    void EntityPublisher::EliminateDuplicateTIDs(TArray<const ComponentType*>& TIDs)
    {
        std::unordered_set<const ComponentType*> set;
        for (const ComponentType* pElement : TIDs)
        {
            set.insert(pElement);
        }

        TIDs.Assign(set.begin(), set.end());
    }
}
