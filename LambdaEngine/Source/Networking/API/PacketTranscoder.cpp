#include "Networking/API/PacketTranscoder.h"
#include "Networking/API/NetworkPacket.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	bool PacketTranscoder::EncodePackets(char* buffer, uint16 bufferSize, std::queue<NetworkPacket*>& packetsToEncode, uint16& bytesWritten, Header* pHeader)
	{
		pHeader->Size = sizeof(Header);
		pHeader->Packets = 0;

		bytesWritten = 0;

		while (!packetsToEncode.empty())
		{
			NetworkPacket* packet = packetsToEncode.front();

			if (packet->GetTotalSize() + pHeader->Size <= bufferSize)
			{
				packetsToEncode.pop();
				pHeader->Size += WritePacket(buffer + pHeader->Size, packet);
				pHeader->Packets++;
			}
			else
			{
				break;
			}
		}

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

	bool PacketTranscoder::DecodePackets(const char* buffer, uint16 bufferSize, std::vector<NetworkPacket*>& packetsDecoded, Header* pHeader)
	{
		uint16 offset = sizeof(Header);

		memcpy(pHeader, buffer, offset);

		if (pHeader->Size != bufferSize)
		{
			LOG_ERROR("[PacketTranscoder]: Received a packet with size missmatch [Exp %d : Rec %d]", pHeader->Size, bufferSize);
			return false;
		}

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

		return messageHeader.Size;
	}
}