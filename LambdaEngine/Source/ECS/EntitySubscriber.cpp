#include "ECS/EntitySubscriber.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    EntitySubscriber::~EntitySubscriber()
    {
        ECSCore* pECS = ECSCore::GetInstance();
        if (pECS && m_SubscriptionID != UINT32_MAX)
        {
            pECS->UnsubscribeFromEntities(m_SubscriptionID);
        }
    }

    void EntitySubscriber::SubscribeToEntities(EntitySubscriberRegistration& subscriberRegistration)
    {
        for (EntitySubscriptionRegistration& subscriptionRegistration : subscriberRegistration.EntitySubscriptionRegistrations)
        {
            ProcessComponentGroups(subscriptionRegistration);
            ProcessExcludedTypes(subscriptionRegistration);
        }

        m_SubscriptionID = ECSCore::GetInstance()->SubscribeToEntities(subscriberRegistration);
    }

    void EntitySubscriber::ProcessComponentGroups(EntitySubscriptionRegistration& subscriptionRegistration)
    {
        // Add the component accesses in the component groups to the component accesses vector
        const TArray<IComponentGroup*>& componentGroups = subscriptionRegistration.ComponentGroups;
        TArray<ComponentAccess>& componentAccesses = subscriptionRegistration.ComponentAccesses;

        for (const IComponentGroup* pComponentGroup : componentGroups)
        {
            const TArray<ComponentAccess> groupAccesses = pComponentGroup->ToArray();
            componentAccesses.Insert(componentAccesses.end(), groupAccesses.begin(), groupAccesses.end());
        }
    }

    void EntitySubscriber::ProcessExcludedTypes(EntitySubscriptionRegistration& subscriptionRegistration)
    {
    #ifdef LAMBDA_DEVELOPMENT
        const TArray<ComponentAccess>& includedTypes = subscriptionRegistration.ComponentAccesses;
        const TArray<const ComponentType*>& excludedTypes = subscriptionRegistration.ExcludedComponentTypes;

        for (const ComponentType* pExcludedComponent : excludedTypes)
        {
            for (const ComponentAccess& includedComponent : includedTypes)
            {
                ASSERT_MSG(pExcludedComponent != includedComponent.TID, "The same component type was both included and excluded in an entity subscription");
            }
        }
    #endif // LAMBDA_DEVELOPMENT
    }
}
