#include "Networking/API/NetworkPacket.h"

namespace LambdaEngine
{
	NetworkPacket::NetworkPacket() : 
		m_SizeOfBuffer(0),
		m_pBuffer(),
		m_Header()
	{

	}

	NetworkPacket::~NetworkPacket()
	{

	}

	NetworkPacket* NetworkPacket::SetType(uint16 type)
	{
		m_Header.Type = type;
		return this;
	}

	uint16 NetworkPacket::GetType() const
	{
		return m_Header.Type;
	}

	char* NetworkPacket::GetBuffer()
	{
		return m_pBuffer;
	}

	const char* NetworkPacket::GetBufferReadOnly() const
	{
		return m_pBuffer;
	}

	uint16 NetworkPacket::GetBufferSize() const
	{
		return m_SizeOfBuffer;
	}

	NetworkPacket::Header& NetworkPacket::GetHeader()
	{
		return m_Header;
	}

	uint8 NetworkPacket::GetHeaderSize() const
	{
		return sizeof(NetworkPacket::Header);
	}

	uint16 NetworkPacket::GetTotalSize() const
	{
		return GetBufferSize() + GetHeaderSize();
	}

	NetworkPacket* NetworkPacket::AppendBytes(uint16 bytes)
	{
		m_SizeOfBuffer += bytes;
		return this;
	}

	uint64 NetworkPacket::GetRemoteSalt() const
	{
		return m_Salt;
	}

	bool NetworkPacket::IsReliable() const
	{
		return m_Header.ReliableUID != 0;
	}

	uint32 NetworkPacket::GetReliableUID() const
	{
		return m_Header.ReliableUID;
	}

	std::string NetworkPacket::ToString() const
	{
		std::string type;
		switch (m_Header.Type)
		{
		case TYPE_UNDEFINED:			type = "TYPE_UNDEFINED";  break;
		case TYPE_PING:					type = "TYPE_PING";  break;
		case TYPE_SERVER_FULL:			type = "TYPE_SERVER_FULL";  break;
		case TYPE_SERVER_NOT_ACCEPTING: type = "TYPE_SERVER_NOT_ACCEPTING";  break;
		case TYPE_CONNNECT:				type = "TYPE_CONNNECT";  break;
		case TYPE_DISCONNECT:			type = "TYPE_DISCONNECT";  break;
		case TYPE_CHALLENGE:			type = "TYPE_CHALLENGE";  break;
		case TYPE_ACCEPTED:				type = "TYPE_ACCEPTED";  break;
		case TYPE_NETWORK_DISCOVERY:	type = "TYPE_NETWORK_DISCOVERY";  break;
		default:
			break;
		}
		return "[Type=" + type + "], [Size=" + std::to_string(GetBufferSize()) + "]";
	}
}