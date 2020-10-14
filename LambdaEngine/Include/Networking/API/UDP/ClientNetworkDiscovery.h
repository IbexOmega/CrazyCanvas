#pragma once

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Networking/API/NetWorker.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/BinaryDecoder.h"

#include "Networking/API/UDP/PacketTransceiverUDP.h"

namespace LambdaEngine
{
	class ISocketUDP;
	class INetworkDiscoveryClient;

	class LAMBDA_API ClientNetworkDiscovery :
		public NetWorker
	{
		friend class NetworkDiscovery;

		struct Packet
		{
			BinaryDecoder Decoder;
			IPEndPoint Sender;
		};

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

		bool HandleReceivedPacket(const IPEndPoint& sender, NetworkSegment* pPacket);
		void FixedTick(Timestamp delta);
		void HandleReceivedPacketsMainThread();

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
		SpinLock m_LockReceivedPackets;
		std::atomic_int8_t m_BufferIndex;
		TArray<Packet> m_ReceivedPackets[2];
	};
}