#include "ECS/EntitySubscriber.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    EntitySubscriptionRegistration::EntitySubscriptionRegistration(TArray<ComponentAccess> componentAccesses, const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoval)
        :pSubscriber(pSubscriber),
        OnEntityAdded(onEntityAdded),
        OnEntityRemoval(onEntityRemoval)
    {
        // Add the component accesses in the component groups to the component accesses vector
        for (const IComponentGroup* pComponentGroup : componentGroups)
        {
            const TArray<ComponentAccess> groupAccesses = pComponentGroup->ToArray();
            componentAccesses.Insert(componentAccesses.end(), groupAccesses.begin(), groupAccesses.end());
        }

        ComponentAccesses = componentAccesses;
    }

    EntitySubscriptionRegistration::EntitySubscriptionRegistration(const TArray<ComponentAccess>& componentAccesses, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoval)
        :EntitySubscriptionRegistration(componentAccesses, {}, pSubscriber, onEntityAdded, onEntityRemoval)
    {}

    EntitySubscriptionRegistration::EntitySubscriptionRegistration(const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded, std::function<void(Entity)>onEntityRemoval)
        :EntitySubscriptionRegistration({}, componentGroups, pSubscriber, onEntityAdded, onEntityRemoval)
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
