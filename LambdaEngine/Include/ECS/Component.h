#pragma once

#include "Containers/TArray.h"
#include "Containers/String.h"
#include "ECS/ComponentType.h"

#define DECL_COMPONENT(Component) \
	private: \
		inline static constexpr const ComponentType s_Type = ComponentType(#Component); \
	public: \
		FORCEINLINE static const ComponentType* Type() \
		{ \
			return &s_Type; \
		}

namespace LambdaEngine
{
	enum ComponentPermissions
	{
		NDA = 0,	// No Data Access
		R   = 1,	// Read
		RW  = 2		// Read & Write
	};

	struct ComponentAccess
	{
		ComponentPermissions Permissions;
		const ComponentType* TID;
	};

	class IComponentGroup
	{
	public:
		virtual TArray<ComponentAccess> ToArray() const = 0;
	};

	template <typename Comp>
	struct ComponentOwnership
	{
		// Called just before deleting a component
		std::function<void(Comp&)> Destructor;
		// Called just before a component array returns a non-const reference to a component
		std::function<void(Comp&)> OnNonConstFetch;
	};
}
