#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IPacketListener.h"

#include "Networking/API/UDP/PacketManagerUDP.h"

namespace LambdaEngine
{
	class ServerUDP;
	class IClientRemoteHandler;
	class PacketTransceiverUDP;

	class LAMBDA_API ClientUDPRemote : 
		public ClientRemoteBase,
		protected IPacketListener
	{
		friend class ServerUDP;
		
	public:
		~ClientUDPRemote();

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
		ClientUDPRemote(uint16 packetPoolSize, uint8 maximumTries, const IPEndPoint& ipEndPoint, ServerUDP* pServer);

		virtual PacketManagerBase* GetPacketManager() override;
		virtual const PacketManagerBase* GetPacketManager() const override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;
		virtual void Tick(Timestamp delta) override;

	private:
		void OnDataReceived(PacketTransceiverBase* pTransciver);
		void SendPackets(PacketTransceiverBase* pTransciver);
		bool HandleReceivedPacket(NetworkSegment* pPacket);

	private:
		ServerUDP* m_pServer;
		PacketManagerUDP m_PacketManager;

		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}