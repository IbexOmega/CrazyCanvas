#pragma once

#include "ECS/Component.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

struct PacketPlayerActionResponseComponent
{
	DECL_COMPONENT_WITH_DIRTY_FLAG(PacketPlayerActionResponseComponent);

#pragma pack(push, 1)
	struct Packet
	{
		int32 SimulationTick = -1;
		glm::vec3 Position;
		glm::vec3 Velocity;
		glm::quat Rotation;
	};
#pragma pack(pop)

	LambdaEngine::TArray<Packet> Packets;
};