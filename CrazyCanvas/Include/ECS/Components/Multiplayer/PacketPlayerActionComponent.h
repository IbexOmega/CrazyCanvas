#pragma once

#include "ECS/Component.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

struct PacketPlayerActionComponent
{
	DECL_COMPONENT_WITH_DIRTY_FLAG(PacketPlayerActionComponent);

#pragma pack(push, 1)
	struct Packet
	{
		int32 SimulationTick = -1;
		glm::quat Rotation;
		int8 DeltaForward = 0;
		int8 DeltaLeft = 0;
	};
#pragma pack(pop)

	LambdaEngine::TArray<Packet> Packets;
};