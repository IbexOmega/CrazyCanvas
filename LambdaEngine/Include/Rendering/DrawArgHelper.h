#pragma once

#include "Defines.h"
#include "ECS/ComponentType.h"

namespace LambdaEngine
{
	class DrawArgHelper
	{
	public:
		DECL_STATIC_CLASS(DrawArgHelper);

		static uint32 FetchComponentDrawArgMask(const ComponentType* type);

	private:
		static THashTable<const ComponentType*, uint32>	s_ComponentToDrawArgMask;
	};
}