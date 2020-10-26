#pragma once

#include "Types.h"

#pragma pack(push, 1)
struct Packet
{
	int32 SimulationTick	= -1;
	int32 NetworkUID		= -1;
};
#pragma pack(pop)