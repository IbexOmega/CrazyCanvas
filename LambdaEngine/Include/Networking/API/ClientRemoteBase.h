#pragma once

#include "Networking/API/IClient.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/ServerBase.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class IClientRemoteHandler;
	class ServerBase;

	struct ClientRemoteDesc : public ServerDesc
	{
		ServerBase* Server = nullptr;
	};

	class LAMBDA_API ClientRemoteBase :
		public IClient,
		protected IPacketListener
	{
		friend class ServerBase;

	public:
		DECL_UNIQUE_CLASS(ClientRemoteBase);
		virtual ~ClientRemoteBase();

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
		ClientRemoteBase(const ClientRemoteDesc& desc);

		void TransmitPackets();
		void DecodeReceivedPackets();

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 tries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries) override;

		virtual PacketTransceiverBase* GetTransceiver() = 0;
		
		void DeleteThis();
		virtual bool CanDeleteNow();
		virtual bool OnTerminationRequested();

	private:
		void ReleaseByServer();
		void Tick(Timestamp delta);
		bool HandleReceivedPacket(NetworkSegment* pPacket);
		void SendDisconnect();
		void SendServerFull();
		void SendServerNotAccepting();

		void RequestTermination();
		void OnTerminationApproved();

	protected:
		bool m_DisconnectedByRemote;

	private:
		IClientRemoteHandler* m_pHandler;
		EClientState m_State;
		ServerBase* m_pServer;
		SpinLock m_Lock;
		Timestamp m_PingInterval;
		Timestamp m_PingTimeout;
		std::atomic_bool m_TerminationRequested;
		std::atomic_bool m_TerminationApproved;
		std::atomic_bool m_ReleasedByServer;
	};
}