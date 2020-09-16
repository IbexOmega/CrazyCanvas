#pragma once

#include "Containers/TArray.h"

#include <typeindex>

#define TID(type) std::type_index(typeid(type))

#define DECL_COMPONENT(Component)		\
	static std::type_index s_TID;

#define INIT_COMPONENT(Component)		\
	std::type_index Component::s_TID = TID(Component);

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
		std::type_index TID;
	};

	class IComponentGroup
	{
	public:
		virtual TArray<ComponentAccess> ToArray() const = 0;
	};
}
