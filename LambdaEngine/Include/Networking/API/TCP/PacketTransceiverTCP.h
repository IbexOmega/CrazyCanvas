#pragma once

#include "Networking/API/PacketTransceiverBase.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class NetworkStatistics;
	class ISocketTCP;

	class LAMBDA_API PacketTransceiverTCP : public PacketTransceiverBase
	{
	public:
		PacketTransceiverTCP();
		~PacketTransceiverTCP();

		virtual void SetSocket(ISocket* pSocket) override;

	protected:
		virtual bool Transmit(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& endPoint) override;
		virtual bool Receive(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& endPoint) override;
		virtual void OnReceiveEnd(const PacketTranscoder::Header& header, TArray<uint32>& newAcks, NetworkStatistics* pStatistics) override;

	private:
		ISocketTCP* m_pSocket;
	};
}