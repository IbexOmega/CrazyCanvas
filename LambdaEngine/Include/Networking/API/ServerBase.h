#pragma once

#include "LambdaEngine.h"

#include "Containers/THashTable.h"
#include "Containers/TSet.h"

#include "Time/API/Timestamp.h"

#include "Networking/API/PacketManagerBase.h"
#include "Networking/API/NetWorker.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/EProtocol.h"

namespace LambdaEngine
{
	class ISocket;
	class IClient;
	class ClientRemoteBase;
	class IPacketListener;
	class IServerHandler;
	class IClientRemoteHandler;

	typedef std::unordered_map<IPEndPoint, ClientRemoteBase*, IPEndPointHasher> ClientMap;
	typedef std::unordered_map<uint64, ClientRemoteBase*>                       ClientUIDMap;

	struct ServerDesc : public PacketManagerDesc
	{
		IServerHandler* Handler = nullptr;
		uint8 MaxClients        = 1;
		EProtocol Protocol      = EProtocol::UDP;
		Timestamp PingInterval  = Timestamp::Seconds(1);
		Timestamp PingTimeout   = Timestamp::Seconds(3);
		bool UsePingSystem      = true;
	};

	class LAMBDA_API ServerBase : public NetWorker
	{
		friend class NetworkUtils;
		friend class ClientRemoteBase;

	public:
		DECL_UNIQUE_CLASS(ServerBase);
		virtual ~ServerBase();

		bool Start(const IPEndPoint& ipEndPoint);
		void Stop(const std::string& reason);
		void Release();
		bool IsRunning();
		const IPEndPoint& GetEndPoint() const;
		void SetAcceptingConnections(bool accepting);
		bool IsAcceptingConnections() const;
		void SetMaxClients(uint8 clients);
		uint8 GetClientCount() const;
		const ServerDesc& GetDescription() const;
		const ClientMap& GetClients() const;
		ClientRemoteBase* GetClient(uint64 uid);
		void SetTimeout(Timestamp time);
		void ResetTimeout();
		
		template<class T>
		bool SendUnreliableStructBroadcast(const T& packet, uint16 packetType, const IClient* pExclude = nullptr)
		{
			std::scoped_lock<SpinLock> lock(m_LockClients);
			bool result = true;
			for (auto& pair : m_Clients)
			{
				IClient* pClient = pair.second;
				if (pClient != pExclude)
					if (!pClient->SendUnreliableStruct<T>(packet, packetType))
						result = false;
			}
			return result;
		}

		template<class T>
		bool SendReliableStructBroadcast(const T& packet, uint16 packetType, IPacketListener* pListener = nullptr, const IClient* pExclude = nullptr)
		{
			std::scoped_lock<SpinLock> lock(m_LockClients);
			bool result = true;
			for (auto& pair : m_Clients)
			{
				IClient* pClient = pair.second;
				if (pClient != pExclude)
					if (!pClient->SendReliableStruct<T>(packet, packetType, pListener))
						result = false;
			}
			return result;
		}

	protected:
		ServerBase(const ServerDesc& desc);

		virtual bool OnThreadsStarted(std::string& reason) override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested(const std::string& reason) override;
		virtual void OnReleaseRequested(const std::string& reason) override;
		virtual void RunTransmitter() override;

		virtual void FixedTick(Timestamp delta);
		ClientRemoteBase* GetClient(const IPEndPoint& endPoint);
		void HandleNewConnection(ClientRemoteBase* pClient);

		virtual ISocket* SetupSocket(std::string& reason) = 0;

	private:
		IClientRemoteHandler* CreateClientHandler() const;
		void OnClientAskForTermination(ClientRemoteBase* pClient);
		bool SendReliableBroadcast(ClientRemoteBase* pClient, NetworkSegment* pPacket, IPacketListener* pListener, bool excludeMySelf = false);
		bool SendUnreliableBroadcast(ClientRemoteBase* pClient, NetworkSegment* pPacket, bool excludeMySelf = false);

	private:
		static void FixedTickStatic(Timestamp timestamp);

	protected:
		ISocket* m_pSocket;

	private:
		IPEndPoint m_IPEndPoint;
		SpinLock m_Lock;
		SpinLock m_LockClients;
		SpinLock m_LockClientVectors;
		ServerDesc m_Description;
		std::atomic_bool m_Accepting;
		ClientMap m_Clients;
		ClientUIDMap m_UIDToClient;
		TArray<ClientRemoteBase*> m_ClientsToAdd;
		TArray<ClientRemoteBase*> m_ClientsToRemove;

	private:
		static std::set<ServerBase*> s_Servers;
		static SpinLock s_Lock;
	};
}