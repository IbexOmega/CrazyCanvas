#include "Networking/API/PacketTranscoder.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/PacketPool.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	bool PacketTranscoder::EncodePackets(char* buffer, uint16 bufferSize, PacketPool* pPacketPool, std::queue<NetworkPacket*>& packetsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader)
	{
		pHeader->Size = sizeof(Header);
		pHeader->Packets = 0;

		bytesWritten = 0;

		std::vector<NetworkPacket*> packetsToFree;

		while (!packetsToEncode.empty())
		{
			NetworkPacket* packet = packetsToEncode.front();

			//Make sure the packet is not bigger than the max size
			ASSERT(packet->GetTotalSize() + sizeof(Header) <= bufferSize);

			if (packet->GetTotalSize() + pHeader->Size <= bufferSize)
			{
				packetsToEncode.pop();
				pHeader->Size += WritePacket(buffer + pHeader->Size, packet);
				pHeader->Packets++;

				if (packet->IsReliable())
					reliableUIDsSent.insert(packet->GetReliableUID());
				else
					packetsToFree.push_back(packet);
			}
			else
			{
				break;
			}
		}

		pPacketPool->FreePackets(packetsToFree);

		memcpy(buffer, pHeader, sizeof(Header));

		bytesWritten = pHeader->Size;
		return packetsToEncode.empty();
	}

	uint16 PacketTranscoder::WritePacket(char* buffer, NetworkPacket* pPacket)
	{
		uint16 headerSize = pPacket->GetHeaderSize();
		uint16 bufferSize = pPacket->GetBufferSize();

		pPacket->GetHeader().Size = pPacket->GetTotalSize();

		memcpy(buffer, &pPacket->GetHeader(), headerSize);
		memcpy(buffer + headerSize, pPacket->GetBufferReadOnly(), bufferSize);

		return headerSize + bufferSize;
	}

	bool PacketTranscoder::DecodePackets(const char* buffer, uint16 bufferSize, PacketPool* pPacketPool, std::vector<NetworkPacket*>& packetsDecoded, Header* pHeader)
	{
		uint16 offset = sizeof(Header);

		memcpy(pHeader, buffer, offset);

		if (pHeader->Size != bufferSize)
		{
			LOG_ERROR("[PacketTranscoder]: Received a packet with size missmatch [Exp %d : Rec %d]", pHeader->Size, bufferSize);
			return false;
		}

		if (!pPacketPool->RequestFreePackets(pHeader->Packets, packetsDecoded))
			return false;

		for (int i = 0; i < pHeader->Packets; i++)
		{
			NetworkPacket* pPacket = packetsDecoded[i];
			offset += ReadPacket(buffer + offset, pPacket);
			pPacket->m_Salt = pHeader->Salt;
		}

		return true;
	}

	uint16 PacketTranscoder::ReadPacket(const char* buffer, NetworkPacket* pPacket)
	{
		NetworkPacket::Header& messageHeader = pPacket->GetHeader();
		uint8 messageHeaderSize = pPacket->GetHeaderSize();

		memcpy(&messageHeader, buffer, messageHeaderSize);
		memcpy(pPacket->GetBuffer(), buffer + messageHeaderSize, messageHeader.Size - messageHeaderSize);
		pPacket->m_SizeOfBuffer = messageHeader.Size - sizeof(NetworkPacket::Header);
		return messageHeader.Size;
	}
}