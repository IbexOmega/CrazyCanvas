#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

#include <atomic>

namespace LambdaEngine
{
	class LAMBDA_API NetworkStatistics
	{
		friend class PacketTransceiver;
		friend class PacketManager;
		friend class PacketManager2;

	public:
		NetworkStatistics();
		~NetworkStatistics();

		uint32 GetPacketsSent()				const;
		uint32 GetMessagesSent()			const;
		uint32 GetReliableMessagesSent()	const;
		uint32 GetPacketsReceived()			const;
		uint32 GetMessagesReceived()		const;
		uint32 GetPacketsLost()				const;
		float64 GetPacketlossRate()			const;
		uint32 GetBytesSent()				const;
		uint32 GetBytesReceived()			const;
		const Timestamp& GetPing()			const;
		uint64 GetSalt()					const;
		uint64 GetRemoteSalt()				const;
		uint32 GetLastReceivedSequenceNr()	const;
		uint32 GetReceivedSequenceBits()	const;
		uint32 GetLastReceivedAckNr()		const;
		uint32 GetReceivedAckBits()			const;
		uint32 GetLastReceivedReliableUID()	const;

	private:
		void Reset();

		uint32 RegisterPacketSent();
		uint32 RegisterMessageSent();
		uint32 RegisterReliableMessageSent();
		void RegisterReliableMessageReceived();
		void SetLastReceivedSequenceNr(uint32 sequence);
		void SetReceivedSequenceBits(uint32 sequenceBits);
		void SetLastReceivedAckNr(uint32 ack);
		void SetReceivedAckBits(uint32 ackBits);
		void SetRemoteSalt(uint64 salt);

	private:
		uint32 m_PacketsLost;
		uint32 m_PacketsSent;
		uint32 m_MessagesSent;
		uint32 m_ReliableMessagesSent;
		uint32 m_PacketsReceived;
		uint32 m_MessagesReceived;
		uint32 m_BytesSent;
		uint32 m_BytesReceived;

		Timestamp m_Ping;

		std::atomic_uint64_t m_Salt;
		std::atomic_uint64_t m_SaltRemote;

		std::atomic_uint32_t m_LastReceivedSequenceNr;
		std::atomic_uint32_t m_ReceivedSequenceBits;

		std::atomic_uint32_t m_LastReceivedAckNr;
		std::atomic_uint32_t m_ReceivedAckBits;

		std::atomic_uint32_t m_LastReceivedReliableUID;
	};
}