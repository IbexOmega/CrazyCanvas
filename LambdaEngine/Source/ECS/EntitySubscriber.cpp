#include "ECS/EntitySubscriber.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    EntitySubscriptionRegistration::EntitySubscriptionRegistration(TArray<ComponentAccess> componentAccesses, const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoved)
        :pSubscriber(pSubscriber),
        OnEntityAdded(onEntityAdded),
        OnEntityRemoved(onEntityRemoved)
    {
        // Add the component accesses in the component groups to the component accesses vector
        for (const IComponentGroup* pComponentGroup : componentGroups)
        {
            const TArray<ComponentAccess> groupAccesses = pComponentGroup->ToVector();
            componentAccesses.Insert(componentAccesses.end(), groupAccesses.begin(), groupAccesses.end());
        }

        ComponentAccesses = componentAccesses;
    }

    EntitySubscriptionRegistration::EntitySubscriptionRegistration(const TArray<ComponentAccess>& componentAccesses, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoved)
        :EntitySubscriptionRegistration(componentAccesses, {}, pSubscriber, onEntityAdded, onEntityRemoved)
    {}

    EntitySubscriptionRegistration::EntitySubscriptionRegistration(const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoved)
        :EntitySubscriptionRegistration({}, componentGroups, pSubscriber, onEntityAdded, onEntityRemoved)
    {}

    EntitySubscriber::EntitySubscriber(ECSCore* pECS)
        :m_pECS(pECS),
        m_SubscriptionID(UINT32_MAX)
    {}

    EntitySubscriber::~EntitySubscriber()
    {
        m_pECS->GetEntityPublisher()->UnsubscribeFromComponents(m_SubscriptionID);
    }

    void EntitySubscriber::SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration, const std::function<bool()>& initFn)
    {
        m_pECS->EnqueueEntitySubscriptions(subscriberRegistration, initFn, &m_SubscriptionID);
    }
}
