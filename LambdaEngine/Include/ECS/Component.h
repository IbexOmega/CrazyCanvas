#pragma once

#include <typeindex>

#define TID(type) std::type_index(typeid(type))

namespace LambdaEngine
{
	// Base class for components
	struct IComponent
	{

	};
}
