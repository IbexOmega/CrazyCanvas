#pragma once

#include "ECS/Component.h"

class Level;

struct LevelRegisteredComponent
{
	DECL_COMPONENT(LevelRegisteredComponent);
	Level* pLevel = nullptr;
};
