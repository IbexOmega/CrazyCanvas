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

	bool PacketTransceiverTCP::Transmit(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint)
	{
		return m_pSocket->Send(pBuffer, bytesToSend, bytesSent);
	}

	bool PacketTransceiverTCP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint)
	{
		return m_pSocket->Receive(pBuffer, size, bytesReceived);
	}

	void PacketTransceiverTCP::OnReceiveEnd(const PacketTranscoder::Header& header, TArray<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		pStatistics->SetLastReceivedSequenceNr(header.Sequence);
		pStatistics->SetLastReceivedAckNr(header.Ack);
		newAcks.PushBack(header.Ack);
	}

	void PacketTransceiverTCP::SetSocket(ISocket* pSocket)
	{
		m_pSocket = (ISocketTCP*)pSocket;
	}
}