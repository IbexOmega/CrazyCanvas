#pragma once
#include "ECS/Component.h"

#include "Multiplayer/Packet/Packet.h"

#define DEFAULT_TIME 5.0f

/*
* Destruction Component
*/
struct DestructionComponent
{
	DECL_COMPONENT(DestructionComponent);
	float TimeLeft = DEFAULT_TIME;
	bool Active = true;
};