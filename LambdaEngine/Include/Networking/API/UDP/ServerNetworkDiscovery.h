#pragma once

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Networking/API/NetWorker.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"

#include "Networking/API/UDP/PacketTransceiverUDP.h"

namespace LambdaEngine
{
    class ISocketUDP;
    class NetworkSegment;
    class INetworkDiscoveryServer;

    class LAMBDA_API ServerNetworkDiscovery : public NetWorker
    {
        friend class NetworkDiscovery;

    public:
        DECL_UNIQUE_CLASS(ServerNetworkDiscovery);
        virtual ~ServerNetworkDiscovery();

        bool Start(const IPEndPoint& endPoint, const String& nameOfGame, uint16 portOfGameServer, INetworkDiscoveryServer* pHandler);
        void Stop(const std::string& reason);
        void Release();
        bool IsRunning();

    private:
        ServerNetworkDiscovery();

        virtual bool OnThreadsStarted(std::string& reason) override;
        virtual void OnThreadsTerminated() override;
        virtual void OnTerminationRequested(const std::string& reason) override;
        virtual void OnReleaseRequested(const std::string& reason) override;
        virtual void RunTransmitter() override;
        virtual void RunReceiver() override;

        void HandleReceivedPacket(const IPEndPoint& sender, NetworkSegment* pPacket);

    private:
        ISocketUDP* m_pSocket;
        IPEndPoint m_IPEndPoint;
        PacketTransceiverUDP m_Transceiver;
        NetworkStatistics m_Statistics;
        SegmentPool m_SegmentPool;
        SpinLock m_Lock;
        uint16 m_PortOfGameServer;
        uint64 m_ServerUID;
        String m_NameOfGame;
        INetworkDiscoveryServer* m_pHandler;
    };
}