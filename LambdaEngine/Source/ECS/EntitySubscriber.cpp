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
            const TArray<ComponentAccess> groupAccesses = pComponentGroup->ToArray();
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

    EntitySubscriber::~EntitySubscriber()
    {
        ECSCore* pECS = ECSCore::GetInstance();
        if (pECS)
            pECS->UnsubscribeFromEntities(m_SubscriptionID);
    }

    void EntitySubscriber::SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration)
    {
        m_SubscriptionID = ECSCore::GetInstance()->SubscribeToEntities(subscriberRegistration);
    }
}
