#pragma once

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Networking/API/NetWorker.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class INetworkDiscoveryClient;

	class LAMBDA_API ClientNetworkDiscovery :
		public NetWorker
	{
		friend class NetworkDiscovery;

	public:
		DECL_UNIQUE_CLASS(ClientNetworkDiscovery);
		virtual ~ClientNetworkDiscovery();

		void Release();
		bool Connect(const IPEndPoint& ipEndPoint, const String& nameOfGame, INetworkDiscoveryClient* pHandler, Timestamp searchInterval);

	protected:
		virtual bool OnThreadsStarted(std::string& reason) override;
		virtual void RunTransmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested(const std::string& reason) override;
		virtual void OnReleaseRequested(const std::string& reason) override;

	private:
		ClientNetworkDiscovery();

		void HandleReceivedPacket(NetworkSegment* pPacket);
		void Tick(Timestamp delta);

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		SegmentPool m_SegmentPool;
		NetworkStatistics m_Statistics;
		PacketTransceiverUDP m_Transceiver;
		String m_NameOfGame;
		SpinLock m_Lock;
		INetworkDiscoveryClient* m_pHandler;
		Timestamp m_TimeOfLastSearch;
		Timestamp m_SearchInterval;
	};
}