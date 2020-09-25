#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	constexpr int16 NETWORK_UID_UNFEDINED = -1;

	struct NetworkComponent
	{
		DECL_COMPONENT(NetworkComponent);

		int32 NetworkUID = NETWORK_UID_UNFEDINED;
	};
}