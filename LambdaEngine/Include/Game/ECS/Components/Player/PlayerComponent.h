#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PlayerBaseComponent
	{
		DECL_COMPONENT(PlayerBaseComponent);

		float WalkSpeedFactor	= 1.4f;
		float SprintSpeedFactor = 1.6f;
	};

	struct PlayerLocalComponent : PlayerBaseComponent
	{
		DECL_COMPONENT(PlayerLocalComponent);
	};

	struct PlayerForeignComponent : PlayerBaseComponent
	{
		DECL_COMPONENT(PlayerForeignComponent);
	};
}