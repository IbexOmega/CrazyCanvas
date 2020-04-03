#include "NetworkPacket.h"

namespace LambdaEngine
{
	NetworkPacket::NetworkPacket(EPacketType packetType) :
		m_Size(sizeof(int16)),
		m_Head(sizeof(int32))
	{
		WriteInt16(packetType);
	}

	void NetworkPacket::WriteInt8(int8 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt16(int16 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt32(int32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteInt64(int64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteFloat32(float32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteFloat64(float64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteBool(bool value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::WriteString(const std::string& value)
	{
		WriteInt16(value.length());
		WriteBuffer(value.c_str(), value.length());
	}

	void NetworkPacket::WriteBuffer(const char* buffer, int16 size)
	{
		memcpy(m_Buffer + m_Size, buffer, size);
		m_Size += size;
	}

	void NetworkPacket::ReadInt8(int8& value)
	{
		ReadBuffer(&value, sizeof(value));
	}

	void NetworkPacket::ReadInt16(int16& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadInt32(int32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadInt64(int64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadFloat32(float32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadFloat64(float64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadBool(bool& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void NetworkPacket::ReadString(std::string& value)
	{
		int16 length;
		ReadInt16(length);
		value.resize(length);
		ReadBuffer(value.data(), length);
	}

	void NetworkPacket::ReadBuffer(char* buffer, int16 bytesToRead)
	{
		memcpy(buffer, m_Buffer + m_Head, bytesToRead);
		m_Head += bytesToRead;
	}

	void NetworkPacket::ResetHead()
	{
		m_Head = sizeof(int32);
	}

	int16 NetworkPacket::GetSize() const
	{
		return m_Size;
	}

	char* NetworkPacket::GetBuffer()
	{
		return m_Buffer;
	}

	void NetworkPacket::Pack()
	{
		memcpy(m_Buffer, (char*)&m_Size, sizeof(m_Size));
	}

	void NetworkPacket::UnPack()
	{
		memcpy(&m_Size, m_Buffer, sizeof(m_Size));
	}
}