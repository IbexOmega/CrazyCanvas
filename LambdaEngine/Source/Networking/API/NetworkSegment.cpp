#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	NetworkSegment::NetworkSegment() : 
		m_SizeOfBuffer(0),
		m_pBuffer(),
		m_Header(),
		m_IsBorrowed(false),
		m_Salt(0)
	{

	}

	NetworkSegment::~NetworkSegment()
	{

	}

	NetworkSegment* NetworkSegment::SetType(uint16 type)
	{
		m_Header.Type = type;

#ifndef LAMBDA_CONFIG_PRODUCTION
		PacketTypeToString(m_Header.Type, m_Type);
#endif

		return this;
	}

	uint16 NetworkSegment::GetType() const
	{
		return m_Header.Type;
	}

	char* NetworkSegment::GetBuffer()
	{
		return m_pBuffer;
	}

	const char* NetworkSegment::GetBufferReadOnly() const
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

	uint8 NetworkSegment::GetHeaderSize() const
	{
		return sizeof(NetworkSegment::Header);
	}

	uint16 NetworkSegment::GetTotalSize() const
	{
		return GetBufferSize() + GetHeaderSize();
	}

	NetworkSegment* NetworkSegment::AppendBytes(uint16 bytes)
	{
		m_SizeOfBuffer += bytes;
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