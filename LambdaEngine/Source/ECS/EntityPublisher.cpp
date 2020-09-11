#include "ECS/EntityPublisher.h"

#include "ECS/ComponentHandler.h"
#include "ECS/System.h"
#include "Log/Log.h"

namespace LambdaEngine
{
    EntityPublisher::EntityPublisher(EntityRegistry* pEntityRegistry)
        :m_pEntityRegistry(pEntityRegistry)
    {}

    void EntityPublisher::RegisterComponentHandler(const ComponentHandlerRegistration& componentHandlerRegistration)
    {
        ComponentHandler* pComponentHandler = componentHandlerRegistration.pComponentHandler;
        std::type_index tidHandler = pComponentHandler->GetHandlerType();

        m_ComponentHandlers[tidHandler] = pComponentHandler;

        // Register component containers
        for (const ComponentRegistration& componentReg : componentHandlerRegistration.ComponentRegistrations)
        {
            auto mapItr = m_ComponentStorage.find(componentReg.TID);

            if (mapItr != m_ComponentStorage.end())
            {
                LOG_WARNING("Attempted to register an already handled component type: %s", componentReg.TID.name());
                continue;
            }

            m_ComponentStorage.insert({componentReg.TID, {componentReg.pComponentContainer, componentReg.ComponentDestructor}});
        }
    }

    void EntityPublisher::DeregisterComponentHandler(ComponentHandler* handler)
    {
        const TArray<std::type_index>& componentTypes = handler->GetHandledTypes();

        for (const std::type_index& componentType : componentTypes)
        {
            // Delete component query functions
            auto handlerItr = m_ComponentStorage.find(componentType);

            if (handlerItr == m_ComponentStorage.end())
            {
                LOG_WARNING("Attempted to deregister a component handler for an unregistered component type: %s", componentType.name());
                continue;
            }

            const TArray<Entity>& entities = handlerItr->second.pContainer->GetIDs();
            for (Entity entity : entities)
            {
                RemovedComponent(entity, componentType);
            }

            m_ComponentStorage.erase(handlerItr);
        }

        auto handlerItr = m_ComponentHandlers.find(handler->GetHandlerType());
        if (handlerItr == m_ComponentHandlers.end())
        {
            LOG_WARNING("Attempted to deregister an unregistered component handler: %s", handler->GetHandlerType().name());
            return;
        }

        m_ComponentHandlers.erase(handlerItr);
    }

    ComponentHandler* EntityPublisher::GetComponentHandler(const std::type_index& handlerType)
    {
        auto itr = m_ComponentHandlers.find(handlerType);

        if (itr == m_ComponentHandlers.end()) {
            LOG_WARNING("Failed to retrieve component handler: %s", handlerType.name());
            return nullptr;
        }

        return itr->second;
    }

    uint32 EntityPublisher::SubscribeToComponents(const EntitySubscriberRegistration& subscriberRegistration)
    {
        // Create subscriptions from the subscription requests by finding the desired component containers
        TArray<EntitySubscription> subscriptions;
        const TArray<EntitySubscriptionRegistration>& subscriptionRequests = subscriberRegistration.EntitySubscriptionRegistrations;
        subscriptions.Reserve(subscriptionRequests.GetSize());

        for (const EntitySubscriptionRegistration& subReq : subscriptionRequests)
        {
            const TArray<ComponentAccess>& componentRegs = subReq.ComponentAccesses;

            EntitySubscription newSub;
            newSub.ComponentTypes.Reserve(componentRegs.GetSize());

            newSub.pSubscriber = subReq.pSubscriber;
            newSub.OnEntityAdded = subReq.OnEntityAdded;
            newSub.OnEntityRemoved = subReq.OnEntityRemoved;

            for (const ComponentAccess& componentReg : componentRegs) {
                auto queryItr = m_ComponentStorage.find(componentReg.TID);

                if (queryItr == m_ComponentStorage.end()) {
                    LOG_ERROR("Attempted to subscribe to unregistered component type: %s, hash: %d", componentReg.TID.name(), componentReg.TID.hash_code());
                    return 0;
                }

                newSub.ComponentTypes.PushBack(componentReg.TID);
            }

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
            const TArray<std::type_index>& componentTypes = subs[subscriptionNr].ComponentTypes;

            for (const std::type_index& componentType : componentTypes)
            {
                m_ComponentSubscriptions.insert({componentType, {subID, subscriptionNr}});
            }
        }

        // A subscription has been made, notify the system of all existing components it subscribed to
        for (EntitySubscription& subscription : subscriptions)
        {
            // Fetch the entity vector of the first subscribed component type
            auto queryItr = m_ComponentStorage.find(subscription.ComponentTypes.GetFront());

            // It has already been ensured that the component type is registered, no need to check for a missed search
            const IDContainer* pComponentContainer = queryItr->second.pContainer;
            const TArray<Entity>& entities = pComponentContainer->GetIDs();

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

    void EntityPublisher::UnsubscribeFromComponents(uint32 subscriptionID)
    {
        if (m_SubscriptionStorage.HasElement(subscriptionID) == false) {
            LOG_WARNING("Attempted to deregistered an unregistered system, ID: %d", subscriptionID);
            return;
        }

        // Use the subscriptions to find and delete component subscriptions
        const TArray<EntitySubscription>& subscriptions = m_SubscriptionStorage.IndexID(subscriptionID);

        for (const EntitySubscription& subscription : subscriptions)
        {
            const TArray<std::type_index>& componentTypes = subscription.ComponentTypes;

            for (const std::type_index& componentType : componentTypes)
            {
                auto subBucketItr = m_ComponentSubscriptions.find(componentType);

                if (subBucketItr == m_ComponentSubscriptions.end())
                {
                    LOG_WARNING("Attempted to delete non-existent component subscription");
                    // The other component subscriptions might exist, so don't return
                    continue;
                }

                // Find the subscription and delete it
                while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == componentType)
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

    void EntityPublisher::NewComponent(Entity entityID, std::type_index componentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_map bucket
        auto subBucketItr = m_ComponentSubscriptions.find(componentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == componentType) {
            // Use indices stored in the component type -> component storage mapping to get the component subscription
            EntitySubscription& sysSub = m_SubscriptionStorage.IndexID(subBucketItr->second.SystemID)[subBucketItr->second.SubIdx];

            if (m_pEntityRegistry->EntityHasTypes(entityID, sysSub.ComponentTypes)) {
                sysSub.pSubscriber->PushBack(entityID);

                if (sysSub.OnEntityAdded) {
                    sysSub.OnEntityAdded(entityID);
                }
            }

            subBucketItr++;
        }
    }

    void EntityPublisher::RemovedComponent(Entity entityID, std::type_index componentType)
    {
        // Get all subscriptions for the component type by iterating through the unordered_map bucket
        auto subBucketItr = m_ComponentSubscriptions.find(componentType);

        while (subBucketItr != m_ComponentSubscriptions.end() && subBucketItr->first == componentType)
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

    void EntityPublisher::EliminateDuplicateTIDs(TArray<std::type_index>& TIDs)
    {
        std::unordered_set<std::type_index> set;
        for (const std::type_index element : TIDs)
        {
            set.insert(element);
        }

        TIDs.Assign(set.begin(), set.end());
    }
}
