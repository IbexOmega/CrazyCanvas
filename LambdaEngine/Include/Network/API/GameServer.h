#pragma once

#include "GameServerBase.h"

#include <unordered_map>

#include "NetworkPacket.h"

namespace LambdaEngine
{
	class RemoteGameClient;

	class LAMBDA_API GameServer : public GameServerBase
	{
	public:
		GameServer();
		~GameServer();

	protected:
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTurminated() override;

	private:
		RemoteGameClient* GetOrCreateClient(const std::string& address, uint64 port);


	private:
		char m_SendBuffer[MAXIMUM_PACKET_SIZE];
		char m_ReceiveBuffer[MAXIMUM_DATAGRAM_SIZE];

		SpinLock m_LockClients;

		std::unordered_map<int64, RemoteGameClient*> m_Clients;

	private:
		static uint64 Hash(const std::string& address, uint64 port);
	};
}
