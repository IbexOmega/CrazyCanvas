#include "Networking/API/NetworkStatistics.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	NetworkStatistics::NetworkStatistics()
	{
		Reset();
	}

	NetworkStatistics::~NetworkStatistics()
	{

	}

	uint32 NetworkStatistics::GetPacketsSent() const
	{
		return m_PacketsSent;
	}

	uint32 NetworkStatistics::GetMessagesSent() const
	{
		return m_MessagesSent;
	}

	uint32 NetworkStatistics::GetPacketsReceived() const
	{
		return m_PacketsReceived;
	}

	uint32 NetworkStatistics::GetMessagesReceived() const
	{
		return m_MessagesReceived;
	}

	uint32 NetworkStatistics::GetPacketsLost() const
	{
		return m_PacketsLost;
	}

	float64 NetworkStatistics::GetPacketlossRate() const
	{
		return m_PacketsLost / (float64)m_PacketsSent;
	}

	uint32 NetworkStatistics::GetBytesSent() const
	{
		return m_BytesSent;
	}

	uint32 NetworkStatistics::GetBytesReceived() const
	{
		return m_BytesReceived;
	}

	const Timestamp& NetworkStatistics::GetPing() const
	{
		return m_Ping;
	}

	uint64 NetworkStatistics::GetSalt() const
	{
		return m_Salt;
	}

	uint64 NetworkStatistics::GetRemoteSalt() const
	{
		return m_SaltRemote;
	}

	void NetworkStatistics::Reset()
	{
		m_Salt				= Random::UInt64();
		m_SaltRemote		= 0;
		m_Ping				= Timestamp::MilliSeconds(10.0f);
		m_PacketsSent		= 0;
		m_MessagesSent		= 0;
		m_PacketsReceived	= 0;
		m_MessagesReceived	= 0;
		m_PacketsLost		= 0;
		m_BytesSent			= 0;
		m_BytesReceived		= 0;
	}
}