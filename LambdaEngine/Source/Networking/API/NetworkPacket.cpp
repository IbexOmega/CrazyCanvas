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

	void NetworkPacket::SetType(uint16 type)
	{
		m_Header.Type = type;
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

	void NetworkPacket::AppendBytes(uint16 bytes)
	{
		m_SizeOfBuffer += bytes;
	}
}