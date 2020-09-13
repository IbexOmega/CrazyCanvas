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
    class ClientRemoteBase;
    class IServerHandler;
    class IClientRemoteHandler;

    struct ServerDesc : public PacketManagerDesc
    {
        IServerHandler* Handler = nullptr;
        uint8 MaxClients        = 1;
        EProtocol Protocol      = EProtocol::UDP;
        Timestamp PingInterval  = Timestamp::Seconds(1);
        Timestamp PingTimeout   = Timestamp::Seconds(2);
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
        bool IsAcceptingConnections();
        uint8 GetClientCount();
        const ServerDesc& GetDescription() const;

    protected:
        ServerBase(const ServerDesc& desc);

        virtual bool OnThreadsStarted(std::string& reason) override;
        virtual void OnThreadsTerminated() override;
        virtual void OnTerminationRequested(const std::string& reason) override;
        virtual void OnReleaseRequested(const std::string& reason) override;
        virtual void RunTransmitter() override;

        virtual void Tick(Timestamp delta);
        void RegisterClient(ClientRemoteBase* pClient);
        ClientRemoteBase* GetClient(const IPEndPoint& endPoint);
        void HandleNewConnection(ClientRemoteBase* pClient);

        virtual ISocket* SetupSocket() = 0;

    private:
        IClientRemoteHandler* CreateClientHandler() const;
        void OnClientAskForTermination(ClientRemoteBase* client);

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
        std::unordered_map<IPEndPoint, ClientRemoteBase*, IPEndPointHasher> m_Clients;
        TArray<ClientRemoteBase*> m_ClientsToAdd;
        TArray<ClientRemoteBase*> m_ClientsToRemove;

    private:
        static std::set<ServerBase*> s_Servers;
        static SpinLock s_Lock;
    };
}