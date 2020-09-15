#include "Networking/API/PacketTranscoder.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/SegmentPool.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	void PacketTranscoder::EncodeSegments(uint8* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, std::queue<NetworkSegment*>& segmentsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader)
	{
		pHeader->Size = sizeof(Header);
		pHeader->Segments = 0;

		bytesWritten = 0;

		TArray<NetworkSegment*> segmentsToFree;

		while (!segmentsToEncode.empty())
		{
			NetworkSegment* segment = segmentsToEncode.front();
			LOG_MESSAGE("PacketTranscoder::EncodeSegments(%s)", segment->ToString().c_str());
			//Make sure the packet is not bigger than the max size
			ASSERT(segment->GetTotalSize() + sizeof(Header) <= bufferSize);

			if (segment->GetTotalSize() + pHeader->Size <= bufferSize)
			{
				segmentsToEncode.pop();
				pHeader->Size += WriteSegment(buffer + pHeader->Size, segment);
				pHeader->Segments++;

				if (segment->IsReliable())
					reliableUIDsSent.insert(segment->GetReliableUID());
				else
					segmentsToFree.PushBack(segment);
			}
			else
			{
				break;
			}
		}

		pSegmentPool->FreeSegments(segmentsToFree);

		memcpy(buffer, pHeader, sizeof(Header));

		bytesWritten = pHeader->Size;
	}

	uint16 PacketTranscoder::WriteSegment(uint8* buffer, NetworkSegment* pSegment)
	{
		uint16 headerSize = pSegment->GetHeaderSize();
		uint16 bufferSize = pSegment->GetBufferSize();

		pSegment->GetHeader().Size = pSegment->GetTotalSize();

		memcpy(buffer, &pSegment->GetHeader(), headerSize);
		memcpy(buffer + headerSize, pSegment->GetBufferReadOnly(), bufferSize);

		return headerSize + bufferSize;
	}

	bool PacketTranscoder::DecodeSegments(const uint8* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, TArray<NetworkSegment*>& segmentsDecoded, Header* pHeader)
	{
		uint16 offset = sizeof(Header);
		memcpy(pHeader, buffer, offset);

		if (pHeader->Size != bufferSize)
		{
			LOG_ERROR("[PacketTranscoder]: Received a packet with size missmatch [Exp %d : Rec %d]", pHeader->Size, bufferSize);
			return false;
		}

		if (!pSegmentPool->RequestFreeSegments(pHeader->Segments, segmentsDecoded))
			return false;

		for (int i = 0; i < pHeader->Segments; i++)
		{
			NetworkSegment* pSegment = segmentsDecoded[i];
			offset += ReadSegment(buffer + offset, pSegment);
			pSegment->m_Salt = pHeader->Salt;

			LOG_MESSAGE("PacketTranscoder::DecodeSegments(%s)", pSegment->ToString().c_str());
		}

		return true;
	}

	uint16 PacketTranscoder::ReadSegment(const uint8* buffer, NetworkSegment* pSegment)
	{
		NetworkSegment::Header& messageHeader = pSegment->GetHeader();
		uint8 messageHeaderSize = pSegment->GetHeaderSize();

		memcpy(&messageHeader, buffer, messageHeaderSize);
		memcpy(pSegment->GetBuffer(), buffer + messageHeaderSize, messageHeader.Size - messageHeaderSize);
		pSegment->m_SizeOfBuffer = messageHeader.Size - sizeof(NetworkSegment::Header);

#ifndef LAMBDA_CONFIG_PRODUCTION
		pSegment->SetType(messageHeader.Type); //Only for debugging, to create a string with the type name
#endif

		return messageHeader.Size;
	}
}