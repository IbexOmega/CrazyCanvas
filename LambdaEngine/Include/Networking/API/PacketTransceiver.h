#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/PacketTranscoder.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class NetworkStatistics;
	class ISocketUDP;

	class LAMBDA_API PacketTransceiver
	{
	public:
		PacketTransceiver();
		~PacketTransceiver();

		int32 Transmit(std::queue<NetworkPacket*>& packets, const IPEndPoint& ipEndPoint, NetworkStatistics* pStatistics);
		bool Receive(std::vector<NetworkPacket*>& packets, IPEndPoint& ipEndPoint, std::vector<uint32>& newAcks, NetworkStatistics* pStatistics);

		void SetSocket(ISocketUDP* pSocket);

	//private:
	public:
		static bool ValidateHeaderSalt(PacketTranscoder::Header* header, NetworkStatistics* pStatistics);
		static void ProcessSequence(uint32 sequence, NetworkStatistics* pStatistics);
		static void ProcessAcks(uint32 ack, uint32 ackBits, NetworkStatistics* pStatistics, std::vector<uint32>& newAcks);

	private:
		ISocketUDP* m_pSocket;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
		char m_pReceiveBuffer[UINT16_MAX];
	};
}