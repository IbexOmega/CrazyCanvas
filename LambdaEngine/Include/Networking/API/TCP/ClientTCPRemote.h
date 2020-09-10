#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IPacketListener.h"

#include "Networking/API/TCP/PacketManagerTCP.h"
#include "Networking/API/TCP/PacketTransceiverTCP.h"

namespace LambdaEngine
{
	class ServerTCP;
	class ISocketTCP;
	class IClientRemoteHandler;
	class PacketTransceiverUDP;

	class LAMBDA_API ClientTCPRemote : 
		public ClientRemoteBase,
		protected IPacketListener
	{
		friend class ServerTCP;
		
	public:
		~ClientTCPRemote();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkSegment* packet) override;
		virtual bool SendReliable(NetworkSegment* packet, IPacketListener* listener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;

	protected:
		ClientTCPRemote(uint16 packetPoolSize, ISocketTCP* pSocket, ServerTCP* pServer);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;
		virtual void Tick(Timestamp delta) override;

	private:
		void OnDataReceived(PacketTransceiverBase* pTransciver);
		void SendPackets();
		bool HandleReceivedPacket(NetworkSegment* pPacket);

		void RunReceiver();
		void OnThreadReceiverTerminated();

	private:
		ServerTCP* m_pServer;
		PacketManagerTCP m_PacketManager;
		PacketTransceiverTCP m_Transceiver;
		Thread* m_pThreadReceiver;
		ISocketTCP* m_pSocket;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}