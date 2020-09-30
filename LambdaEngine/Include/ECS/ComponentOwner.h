#pragma once

#include "ECS/Component.h"
#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	class ComponentOwner
	{
	public:
		ComponentOwner() = default;
		~ComponentOwner();

		template <typename Comp>
		void SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership);

	private:
		TArray<const ComponentType*> m_OwnedComponentTypes;
	};

	template <typename Comp>
	inline void ComponentOwner::SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership)
	{
		ECSCore::GetInstance()->SetComponentOwner(componentOwnership);
		m_OwnedComponentTypes.PushBack(Comp::Type());
	}
}
