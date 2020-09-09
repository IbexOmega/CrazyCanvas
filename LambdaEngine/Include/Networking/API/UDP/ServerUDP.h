#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/ServerBase.h"

#include "Networking/API/UDP/PacketManagerUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class ClientUDPRemote;
	class IServerHandler;
	class IClientRemoteHandler;

	struct ServerUDPDesc : public PacketManagerDesc
	{
		IServerHandler* Handler	= nullptr;
		uint8 MaxClients			= 1;
	};

	class LAMBDA_API ServerUDP : public ServerBase
	{
		friend class ClientUDPRemote;

	public:
		~ServerUDP();

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	protected:
		ServerUDP(const ServerUDPDesc& desc);

		virtual ISocket* SetupSocket() override;
		virtual void RunReceiver() override;
		virtual void TransmitPacketsForClient(ClientRemoteBase* pClient) override;

	private:
		IClientRemoteHandler* CreateClientHandler();
		ClientUDPRemote* GetOrCreateClient(const IPEndPoint& sender, bool& newConnection);
		void OnClientDisconnected(ClientUDPRemote* client, bool sendDisconnectPacket);
		void SendDisconnect(ClientUDPRemote* client);
		void SendServerFull(ClientUDPRemote* client);
		void SendServerNotAccepting(ClientUDPRemote* client);

	public:
		static ServerUDP* Create(const ServerUDPDesc& desc);

	private:
		ISocketUDP* m_pSocket;
		PacketTransceiverUDP m_Transciver;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		ServerUDPDesc m_Desc;
		float m_PacketLoss;
	};
}