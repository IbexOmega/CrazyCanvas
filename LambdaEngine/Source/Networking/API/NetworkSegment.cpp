#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	NetworkSegment::NetworkSegment() : 
		m_SizeOfBuffer(0),
		m_pBuffer(),
		m_Header(),
		m_Salt(0),
		m_ReadHead(0)
#ifdef LAMBDA_CONFIG_DEBUG
		, m_IsBorrowed(false)
#endif
	{

	}

	NetworkSegment::~NetworkSegment()
	{

	}

	NetworkSegment* NetworkSegment::SetType(uint16 type)
	{
		m_Header.Type = type;

#ifdef LAMBDA_CONFIG_DEBUG
		PacketTypeToString(m_Header.Type, m_Type);
#endif

		return this;
	}

	uint16 NetworkSegment::GetType() const
	{
		return m_Header.Type;
	}

	const uint8* NetworkSegment::GetBuffer() const
	{
		return m_pBuffer;
	}

	uint16 NetworkSegment::GetBufferSize() const
	{
		return m_SizeOfBuffer;
	}

	NetworkSegment::Header& NetworkSegment::GetHeader()
	{
		return m_Header;
	}

	uint16 NetworkSegment::GetTotalSize() const
	{
		return GetBufferSize() + HeaderSize;
	}

	NetworkSegment* NetworkSegment::Write(const void* pBuffer, uint16 bytes)
	{
		ASSERT(bytes > 0);
		ASSERT(m_SizeOfBuffer + bytes <= MAXIMUM_SEGMENT_SIZE);
		memcpy(m_pBuffer + m_SizeOfBuffer, pBuffer, bytes);
		m_SizeOfBuffer += bytes;
		return this;
	}

	NetworkSegment* NetworkSegment::Read(void* pBuffer, uint16 bytes)
	{
		ASSERT(bytes > 0);
		ASSERT(m_ReadHead + bytes <= m_SizeOfBuffer);
		memcpy(pBuffer, m_pBuffer + m_ReadHead, bytes);
		m_ReadHead += bytes;
		return this;
	}

	uint64 NetworkSegment::GetRemoteSalt() const
	{
		return m_Salt;
	}

	bool NetworkSegment::IsReliable() const
	{
		return m_Header.ReliableUID != 0;
	}

	uint32 NetworkSegment::GetReliableUID() const
	{
		return m_Header.ReliableUID;
	}

	std::string NetworkSegment::ToString() const
	{
		std::string type;
		PacketTypeToString(m_Header.Type, type);
		return "[Type=" + type + "], [Size=" + std::to_string(GetBufferSize()) + "], [UID=" + std::to_string(m_Header.UID) + "]";
	}

	void NetworkSegment::CopyTo(NetworkSegment* pSegment) const
	{
		if (pSegment == this)
			return;

		memcpy(&(pSegment->m_Header), &m_Header, sizeof(Header));
		memcpy(pSegment->m_pBuffer, m_pBuffer, m_SizeOfBuffer);
		pSegment->m_SizeOfBuffer = m_SizeOfBuffer;
		pSegment->m_Salt = m_Salt;
	}

	void NetworkSegment::ResetReadHead()
	{
		m_ReadHead = 0;
	}

	void NetworkSegment::PacketTypeToString(uint16 type, std::string& str)
	{
		switch (type)
		{
		case TYPE_UNDEFINED:			str = "TYPE_UNDEFINED";  break;
		case TYPE_PING:					str = "TYPE_PING";  break;
		case TYPE_SERVER_FULL:			str = "TYPE_SERVER_FULL";  break;
		case TYPE_SERVER_NOT_ACCEPTING: str = "TYPE_SERVER_NOT_ACCEPTING";  break;
		case TYPE_CONNNECT:				str = "TYPE_CONNNECT";  break;
		case TYPE_DISCONNECT:			str = "TYPE_DISCONNECT";  break;
		case TYPE_CHALLENGE:			str = "TYPE_CHALLENGE";  break;
		case TYPE_ACCEPTED:				str = "TYPE_ACCEPTED";  break;
		case TYPE_NETWORK_ACK:			str = "TYPE_NETWORK_ACK";  break;
		case TYPE_NETWORK_DISCOVERY:	str = "TYPE_NETWORK_DISCOVERY";  break;
		default:						str = "USER_PACKET(" + std::to_string(type) + ")"; break;
		}
	}
}