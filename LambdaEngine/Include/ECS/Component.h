#pragma once

#include "Containers/TArray.h"

#include <typeindex>

namespace LambdaEngine
{
	struct _Internal
	{
		static uint32 s_Generator;
	};
}

#define TID(type) std::type_index(typeid(type))
#define GENERATE_MASK() (1 << (_Internal::s_Generator++))

#define DECL_COMPONENT(Component)		\
	static std::type_index s_TID;		\
	static uint32 s_DrawArgMask

#define DECL_DRAW_ARG_COMPONENT(Component)	DECL_COMPONENT(Component)

#define INIT_COMPONENT(Component)						\
	std::type_index Component::s_TID = TID(Component);  \
	uint32 Component::s_DrawArgMask = 0

#define INIT_DRAW_ARG_COMPONENT(Component)						\
	std::type_index Component::s_TID = TID(Component);  \
	uint32 Component::s_DrawArgMask = GENERATE_MASK();

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
