#pragma once

#include "GameServerBase.h"

#include "NetworkPacket.h"

#include <vector>

namespace LambdaEngine
{
	class IGameClientHandler;
	class GameServer;

	class LAMBDA_API RemoteGameClient
	{
		friend class GameServer;

	public:
		~RemoteGameClient();

		bool AddPacket(NetworkPacket* packet);
		const std::string& GetAddress() const;
		uint16 GetPort() const;
		uint64 GetHash() const;

	private:
		RemoteGameClient(const std::string& address, uint16 port, uint64 hash, GameServer* pGameServer, IGameClientHandler* handler);

		std::vector<NetworkPacket*>* SwapAndGetPacketBuffer();
		bool TransmitPackets(char* buffer, int32 size);
		void OnPacketReceived(char* buffer, int32 bytesReceived);
		void ExtractPackets(char* buffer, uint8 packets);

	private:
		std::vector<NetworkPacket*>* m_PacketBuffers[2];
		std::atomic_int m_PacketBufferIndex;

		SpinLock m_LockPacketBuffers;

		GameServer* m_pGameServer;

		IGameClientHandler* m_pHandler;

		std::string m_Address;
		uint16 m_Port;
		uint64 m_Hash;

	private:
		static NetworkPacket* RemovePacketOfSizeLOE(std::vector<NetworkPacket*>* packets, PACKET_SIZE size);
	};
}
