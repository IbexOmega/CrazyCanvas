#pragma once

#include "ECS/Component.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"

constexpr const uint32 MAX_NUM_TEAMS = 3;

struct TeamComponent
{
	DECL_COMPONENT(TeamComponent);
	uint32 TeamIndex = 0;
};
