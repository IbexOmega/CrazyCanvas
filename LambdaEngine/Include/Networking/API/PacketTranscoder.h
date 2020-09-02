#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class PacketPool;

	class LAMBDA_API PacketTranscoder
	{
	public:
#pragma pack(push, 1)
		struct Header
		{
			uint16 Size = 0;
			uint64 Salt = 0;
			uint32 Sequence = 0;
			uint32 Ack = 0;
			uint32 AckBits = 0;
			uint8  Packets = 0;
		};
#pragma pack(pop)

	public:
		DECL_STATIC_CLASS(PacketTranscoder);

		static bool EncodePackets(char* buffer, uint16 bufferSize, PacketPool* pPacketPool, std::queue<NetworkPacket*>& packetsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader);
		static bool DecodePackets(const char* buffer, uint16 bufferSize, PacketPool* pPacketPool, TArray<NetworkPacket*>& packetsDecoded, Header* pHeader);

	private:
		static uint16 WritePacket(char* buffer, NetworkPacket* pPacket);
		static uint16 ReadPacket(const char* buffer, NetworkPacket* pPacket);
	};
}