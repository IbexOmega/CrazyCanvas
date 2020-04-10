#pragma once

#include "GameServerBase.h"

#include <queue>

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API GameServer : public GameServerBase
	{
	public:
		GameServer();
		~GameServer();

		bool AddPacket(NetworkPacket* packet);

	protected:
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTurminated() override;

	private:
		std::queue<NetworkPacket*>* SwapAndGetPacketBuffer();
		bool TransmitPackets(std::queue<NetworkPacket*>* packets);

	private:
		std::queue<NetworkPacket*>* m_PacketBuffers[2];
		std::atomic_int m_PacketBufferIndex;

		SpinLock m_LockPacketBuffers;

		//char m_SendBuffer[MAXIMUM_DATAGRAM_SIZE];
	};
}
