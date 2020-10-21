#include "Networking/API/PacketTranscoder.h"
#include "Networking/API/SegmentPool.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	void PacketTranscoder::EncodeSegments(uint8* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, std::set<NetworkSegment*, NetworkSegmentUIDOrder>& segmentsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader)
	{
		pHeader->Size = sizeof(Header);
		pHeader->Segments = 0;

		bytesWritten = 0;

		TArray<NetworkSegment*> segmentsToFree;

		for (auto it = segmentsToEncode.begin(); it != segmentsToEncode.end();)
		{
			NetworkSegment* pSegment = *it;
			//LOG_ERROR("PacketTranscoder::EncodeSegments(%s)", pSegment->ToString().c_str());

			ASSERT(pSegment->GetTotalSize() + sizeof(Header) <= bufferSize);

			if (pSegment->GetTotalSize() + pHeader->Size <= bufferSize)
			{
				it = segmentsToEncode.erase(it);
				pHeader->Size += WriteSegment(buffer + pHeader->Size, pSegment);
				pHeader->Segments++;

				if (pSegment->IsReliable())
					reliableUIDsSent.insert(pSegment->GetReliableUID());
				else
					segmentsToFree.PushBack(pSegment);
			}
			else
			{
				break;
			}
		}

		pSegmentPool->FreeSegments(segmentsToFree, "PacketTranscoder::EncodeSegments");

		memcpy(buffer, pHeader, sizeof(Header));

		bytesWritten = pHeader->Size;
	}

	uint16 PacketTranscoder::WriteSegment(uint8* buffer, NetworkSegment* pSegment)
	{
		static constexpr uint8 headerSize = NetworkSegment::HeaderSize;
		uint16 bufferSize = pSegment->GetBufferSize();

		pSegment->GetHeader().Size = pSegment->GetTotalSize();

		memcpy(buffer, &pSegment->GetHeader(), headerSize);
		memcpy(buffer + headerSize, pSegment->GetBuffer(), bufferSize);

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

#ifdef LAMBDA_CONFIG_DEBUG
		if (!pSegmentPool->RequestFreeSegments(pHeader->Segments, segmentsDecoded, "PacketTranscoder"))
			return false;
#else
		if (!pSegmentPool->RequestFreeSegments(pHeader->Segments, segmentsDecoded))
			return false;
#endif
			

		for (int i = 0; i < pHeader->Segments; i++)
		{
			NetworkSegment* pSegment = segmentsDecoded[i];
			offset += ReadSegment(buffer + offset, pSegment);
			pSegment->m_Salt = pHeader->Salt;

			//LOG_ERROR("PacketTranscoder::DecodeSegments(%s)", pSegment->ToString().c_str());
		}

		return true;
	}

	uint16 PacketTranscoder::ReadSegment(const uint8* pBuffer, NetworkSegment* pSegment)
	{
		NetworkSegment::Header& messageHeader = pSegment->GetHeader();
		static constexpr uint8 segmentHeaderSize = NetworkSegment::HeaderSize;

		uint8* pSegmentBuffer = const_cast<uint8*>(pSegment->GetBuffer());

		memcpy(&messageHeader, pBuffer, segmentHeaderSize);
		memcpy(pSegmentBuffer, pBuffer + segmentHeaderSize, messageHeader.Size - segmentHeaderSize);
		pSegment->m_SizeOfBuffer = messageHeader.Size - segmentHeaderSize;

#ifndef LAMBDA_CONFIG_PRODUCTION
		pSegment->SetType(messageHeader.Type); //Only for debugging, to create a string with the type name
#endif

		return messageHeader.Size;
	}
}