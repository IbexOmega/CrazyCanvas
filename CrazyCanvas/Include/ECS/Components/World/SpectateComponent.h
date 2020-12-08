#pragma once
#include "ECS/Component.h"

enum class SpectateType
{
	FLAG_SPAWN,
	LOCAL_PLAYER,
	PLAYER,
	SPECTATE_OBJECT
};

struct SpectateComponent
{
	DECL_COMPONENT(SpectateComponent);
	SpectateType SpectateType;
};