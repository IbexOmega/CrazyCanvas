#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

#include <functional>

namespace LambdaEngine
{
	class IDVector;

	// EntitySubscriptionRegistration contains all required information to request a single entity subscription
	class EntitySubscriptionRegistration
	{
	public:
		EntitySubscriptionRegistration(TArray<ComponentAccess> componentAccesses, const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoval = nullptr);
		EntitySubscriptionRegistration(const TArray<ComponentAccess>& componentAccesses, const TArray<IComponentGroup*>& componentGroups, const TArray<std::type_index>& excludedComponentTypes, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoval = nullptr);
		EntitySubscriptionRegistration(const TArray<ComponentAccess>& componentAccesses, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoval = nullptr);
		EntitySubscriptionRegistration(const TArray<IComponentGroup*>& componentGroups, IDVector* pSubscriber, std::function<void(Entity)>onEntityAdded = nullptr, std::function<void(Entity)>onEntityRemoval = nullptr);

	public:
		TArray<ComponentAccess> ComponentAccesses;
		TArray<std::type_index> ExcludedComponentTypes;
		IDVector* pSubscriber;
		// Optional: Called after an entity is added due to the subscription
		std::function<void(Entity)> OnEntityAdded;
		// Optional: Called before an entity is removed
		std::function<void(Entity)> OnEntityRemoval;
	};

	// EntitySubscriberRegistration is a complete set of data required to register a component subscriber
	struct EntitySubscriberRegistration
	{
		TArray<EntitySubscriptionRegistration> EntitySubscriptionRegistrations;
		//TArray<std::type_index> ExcludedComponentTypes;
		/*  AdditionalDependencies are components that the subscriber will process.
			However, the subscriber will not store an array of the entities whose components it will process. */
		TArray<ComponentAccess> AdditionalDependencies;
	};

	// EntitySubscriber deregisters its entity subscriptions at destruction
	class EntitySubscriber
	{
	public:
		EntitySubscriber() = default;
		~EntitySubscriber();

		// subscribeToEntities enqueues entity subscriptions. initFn is called when all dependencies have been initialized.
		void SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration);

	private:
		uint32 m_SubscriptionID = UINT32_MAX;
	};
}
