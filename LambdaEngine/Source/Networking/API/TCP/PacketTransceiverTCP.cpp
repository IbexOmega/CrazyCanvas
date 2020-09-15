#include "Networking/API/NetworkStatistics.h"

#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/TCP/PacketTransceiverTCP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketTransceiverTCP::PacketTransceiverTCP() :
		m_pSocket(nullptr)
	{

	}

	PacketTransceiverTCP::~PacketTransceiverTCP()
	{

	}

	bool PacketTransceiverTCP::Transmit(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& endPoint)
	{
		UNREFERENCED_VARIABLE(endPoint);
		return m_pSocket->Send(pBuffer, bytesToSend, bytesSent);
	}

	bool PacketTransceiverTCP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& endPoint)
	{
		UNREFERENCED_VARIABLE(endPoint);

		static const uint32 headerSize = sizeof(PacketTranscoder::Header);

		if (!ForceReceive(pBuffer, headerSize))
			return false;
		
		PacketTranscoder::Header* header = (PacketTranscoder::Header*)pBuffer;
		if (header->Size > size)
			return false;

		if (!ForceReceive(pBuffer + headerSize, header->Size - headerSize))
			return false;

		bytesReceived = header->Size;
		return true;
	}

	bool PacketTransceiverTCP::ForceReceive(uint8* pBuffer, uint32 bytesToRead)
	{
		uint32 totalBytesRead = 0;
		int32 bytesRead = 0;
		while (totalBytesRead != bytesToRead)
		{
			if (!m_pSocket->Receive(pBuffer + totalBytesRead, bytesToRead - totalBytesRead, bytesRead))
				return false;
			totalBytesRead += bytesRead;
		}
		return true;
	}

	void PacketTransceiverTCP::OnReceiveEnd(PacketTranscoder::Header* pHeader, TArray<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		pStatistics->SetLastReceivedSequenceNr(pHeader->Sequence);
		pStatistics->SetLastReceivedAckNr(pHeader->Ack);
		newAcks.PushBack(pHeader->Ack);
	}

	void PacketTransceiverTCP::SetSocket(ISocket* pSocket)
	{
		m_pSocket = (ISocketTCP*)pSocket;
	}
}