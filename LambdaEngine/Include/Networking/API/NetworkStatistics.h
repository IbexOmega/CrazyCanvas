#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

#include <atomic>

namespace LambdaEngine
{
	class LAMBDA_API NetworkStatistics
	{
		friend class PacketManager;

	public:
		NetworkStatistics();
		~NetworkStatistics();

		uint32 GetPacketsSent()			const;
		uint32 GetMessagesSent()		const;
		uint32 GetPacketsReceived()		const;
		uint32 GetMessagesReceived()	const;
		uint32 GetPacketsLost()			const;
		float64 GetPacketlossRate()		const;
		uint32 GetBytesSent()			const;
		uint32 GetBytesReceived()		const;
		const Timestamp& GetPing()		const;
		uint64 GetSalt()				const;
		uint64 GetRemoteSalt()			const;

	private:
		void Reset();

	private:
		uint32 m_PacketsLost;
		uint32 m_PacketsSent;
		uint32 m_MessagesSent;
		uint32 m_PacketsReceived;
		uint32 m_MessagesReceived;
		uint32 m_BytesSent;
		uint32 m_BytesReceived;

		Timestamp m_Ping;

		std::atomic_uint64_t m_Salt;
		std::atomic_uint64_t m_SaltRemote;
	};
}