#pragma once

#include "Networking/API/PacketTransceiverBase.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class NetworkStatistics;
	class ISocketUDP;

	class LAMBDA_API PacketTransceiverUDP : public PacketTransceiverBase
	{
	public:
		PacketTransceiverUDP();
		~PacketTransceiverUDP();

		virtual void SetSocket(ISocket* pSocket) override;

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		virtual bool TransmitData(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint) override;
		virtual bool ReceiveData(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint) override;
		virtual void OnReceiveEnd(PacketTranscoder::Header* pHeader, TArray<uint32>& newAcks, NetworkStatistics* pStatistics) override;

	private:
		static void ProcessSequence(uint32 sequence, NetworkStatistics* pStatistics);
		static void ProcessAcks(uint32 ack, uint64 ackBits, NetworkStatistics* pStatistics, TArray<uint32>& newAcks);

	private:
		ISocketUDP* m_pSocket;
		float32 m_ReceivingLossRatio;
		float32 m_TransmittingLossRatio;
	};
}