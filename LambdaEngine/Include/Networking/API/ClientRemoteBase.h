#pragma once

#include "Networking/API/IClient.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/ServerBase.h"

#include "Time/API/Timestamp.h"

#include "Containers/THashTable.h"

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
		friend class NetworkUtils;

	public:
		DECL_UNIQUE_CLASS(ClientRemoteBase);
		virtual ~ClientRemoteBase();

		virtual void Disconnect(const std::string& reason) override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkSegment* pPacket) override;
		virtual bool SendReliable(NetworkSegment* pPacket, IPacketListener* pListener = nullptr) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;
		virtual IClientRemoteHandler* GetHandler() override;
		virtual uint64 GetUID() const override;

		bool SendUnreliableBroadcast(NetworkSegment* pPacket, bool excludeMySelf = false);
		bool SendReliableBroadcast(NetworkSegment* pPacket, IPacketListener* pListener = nullptr, bool excludeMySelf = false);
		const ClientMap& GetClients() const;
		ServerBase* GetServer();

		template<class T>
		bool SendUnreliableStructBroadcast(const T& packet, uint16 packetType, bool excludeMySelf = false)
		{
			return GetServer()->SendUnreliableStructBroadcast<T>(packet, packetType, excludeMySelf ? this : nullptr);
		}

		template<class T>
		bool SendReliableStructBroadcast(const T& packet, uint16 packetType, IPacketListener* pListener = nullptr, bool excludeMySelf = false)
		{
			return GetServer()->SendReliableStructBroadcast<T>(packet, packetType, pListener, excludeMySelf ? this : nullptr);
		}

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
		void FixedTick(Timestamp delta);
		void UpdatePingSystem();
		void HandleReceivedPacketsMainThread();
		bool HandleReceivedPacket(NetworkSegment* pPacket);
		void SendDisconnect();
		void SendServerFull();
		void SendServerNotAccepting();

		bool RequestTermination(const std::string& reason, bool byServer = false);
		void OnTerminationApproved();
		void OnConnectionApproved();

	private:
		static void FixedTickStatic(Timestamp timestamp);
		static void ReleaseStatic();

	protected:
		bool m_DisconnectedByRemote;

	private:
		IClientRemoteHandler* m_pHandler;
		EClientState m_State;
		ServerBase* m_pServer;
		SpinLock m_Lock;
		Timestamp m_PingInterval;
		Timestamp m_PingTimeout;
		Timestamp m_LastPingTimestamp;
		std::atomic_bool m_TerminationRequested;
		std::atomic_bool m_TerminationApproved;
		bool m_UsePingSystem;
		SpinLock m_LockReceivedPackets;
		std::atomic_int8_t m_BufferIndex;
		TArray<NetworkSegment*> m_ReceivedPackets[2];

	private:
		static SpinLock s_LockStatic;
		static std::atomic_int s_Instances;
		static THashTable<ClientRemoteBase*, uint8> s_ClientRemoteBasesToDelete;
	};
}