#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

#include <atomic>

namespace LambdaEngine
{
	class LAMBDA_API NetworkStatistics
	{
		friend class PacketTransceiverBase;
		friend class PacketTransceiverUDP;
		friend class PacketTransceiverTCP;
		friend class PacketManagerUDP;
		friend class PacketManagerBase;
		friend class ClientBase;
		friend class ClientRemoteBase;
		friend class ClientNetworkDiscovery;

	public:
		NetworkStatistics();
		~NetworkStatistics();

		/*
		* return - The number of physical packets sent
		*/
		uint32 GetPacketsSent()	const;

		/*
		* return - The number of Segments Registered (Unique Segments)
		*/
		uint32 GetSegmentsRegistered() const;

		/*
		* return - The number of total Segments Sent
		*/
		uint32 GetSegmentsSent() const;

		/*
		* return - The number of reliable Segments sent
		*/
		uint32 GetReliableSegmentsSent() const;

		/*
		* return - The number of physical packets received
		*/
		uint32 GetPacketsReceived()	const;

		/*
		* return - The number of packets (NetworkPacket) received
		*/
		uint32 GetSegmentsReceived() const;

		/*
		* return - The number of physical packets lost (Packets we never received)
		*/
		uint32 GetReceivingPacketsLost() const;

		/*
		* return - The number of physical packets lost (Packets the remote never received)
		*/
		uint32 GetSendingPacketsLost() const;

		/*
		* return - The percentage of physical packets lost (Packets we never received)
		*/
		float64 GetReceivingPacketLossRate() const;

		/*
		* return - The percentage of physical packets lost (Packets the remote never received)
		*/
		float64 GetSendingPacketLossRate() const;

		/*
		* return - The total number of bytes sent
		*/
		uint32 GetBytesSent() const;

		/*
		* return - The total number of bytes received
		*/
		uint32 GetBytesReceived() const;

		/*
		* return - The avarage roun trip time of the 10 latest physical packets
		*/
		float64 GetPing() const;

		/*
		* return - The unique salt representing this side of the connection
		*/
		uint64 GetSalt() const;

		/*
		* return - The unique salt representing the remote side of the connection
		*/
		uint64 GetRemoteSalt() const;

		/*
		* return - The timestamp of when the last physical packet was sent
		*/
		Timestamp GetTimestampLastSent() const;

		/*
		* return - The timestamp of when the last physical packet was received
		*/
		Timestamp GetTimestampLastReceived()	const;

		

		uint32 GetLastReceivedSequenceNr()	const;
		uint64 GetReceivedSequenceBits()	const;
		uint32 GetLastReceivedAckNr()		const;
		uint64 GetReceivedAckBits()			const;
		uint32 GetLastReceivedReliableUID()	const;
		uint32 GetSegmentsResent()			const;

	private:
		void Reset();

		uint32 RegisterPacketSent();
		uint32 RegisterUniqueSegment();
		void RegisterSegmentSent(uint32 segments);
		uint32 RegisterReliableSegmentSent();
		void RegisterPacketReceived(uint32 segments, uint32 bytes);
		void RegisterReliableSegmentReceived();

		void SetPacketsSentByRemote(uint32 packets);
		void SetPacketsReceivedByRemote(uint32 packetsLost);
		void RegisterBytesSent(uint32 bytes);
		void SetRemoteSalt(uint64 salt);

		void SetLastReceivedSequenceNr(uint32 sequence);
		void SetReceivedSequenceBits(uint64 sequenceBits);
		void SetLastReceivedAckNr(uint32 ack);
		void SetReceivedAckBits(uint64 ackBits);

		void UpdatePacketsSentFixed();

		void RegisterSegmentResent();

	private:
		uint32 m_PacketsSentByRemote;
		uint32 m_PacketReceivedByRemote;
		uint32 m_PacketsSentFixed;
		uint32 m_PacketsReceivedFixed;

		uint32 m_PacketsSent;
		std::atomic_uint32_t m_SegmentsRegistered;
		uint32 m_SegmentsSent;
		std::atomic_uint32_t m_ReliableSegmentsSent;
		uint32 m_PacketsReceived;
		uint32 m_SegmentsReceived;
		uint32 m_BytesSent;
		uint32 m_BytesReceived;
		uint32 m_SegmentsResent;

		float64 m_Ping;
		Timestamp m_TimestampLastSent;
		Timestamp m_TimestampLastReceived;

		std::atomic_uint64_t m_Salt;
		std::atomic_uint64_t m_SaltRemote;

		std::atomic_uint32_t m_LastReceivedSequenceNr;
		std::atomic_uint64_t m_ReceivedSequenceBits;

		std::atomic_uint32_t m_LastReceivedAckNr;
		std::atomic_uint64_t m_ReceivedAckBits;

		std::atomic_uint32_t m_LastReceivedReliableUID;
	};
}