#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class SegmentPool;

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
			uint8  Segments = 0;
		};
#pragma pack(pop)

	public:
		DECL_STATIC_CLASS(PacketTranscoder);

		static bool EncodeSegments(char* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, std::queue<NetworkSegment*>& segmentsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader);
		static bool DecodeSegments(const char* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, TArray<NetworkSegment*>& segmentsDecoded, Header* pHeader);

	private:
		static uint16 WriteSegment(char* buffer, NetworkSegment* pSegment);
		static uint16 ReadSegment(const char* buffer, NetworkSegment* pSegment);
	};
}