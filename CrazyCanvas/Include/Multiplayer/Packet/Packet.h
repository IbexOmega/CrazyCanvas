#pragma once
#include "Types.h"

#define DECL_PACKET(Packet) \
	friend class PacketType; \
	private: \
		inline static uint16 s_Type = 0; \
	public: \
		FORCEINLINE static uint16 Type() \
		{ \
			return s_Type; \
		} \

#pragma pack(push, 1)
struct Packet
{
	int32 SimulationTick	= -1;
	int32 NetworkUID		= -1;
};
#pragma pack(pop)