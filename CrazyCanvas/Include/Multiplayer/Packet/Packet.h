#pragma once
#include "Types.h"

#define DECL_PACKET(Packet) \
	friend class PacketType; \
	private: \
		inline static uint16 s_Type = 0; \
		inline static const char* s_Name = #Packet; \
	public: \
		FORCEINLINE static uint16 GetType() \
		{ \
			return s_Type; \
		} \
		FORCEINLINE static const char* GetName() \
		{ \
			return s_Name; \
		} \

#pragma pack(push, 1)
struct Packet
{
	int32 SimulationTick	= -1;
	int32 NetworkUID		= -1;
};
#pragma pack(pop)