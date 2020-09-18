#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	BinaryEncoder::BinaryEncoder(NetworkSegment* packet) : 
		m_pNetworkPacket(packet)
	{
		
	}

	BinaryEncoder::~BinaryEncoder()
	{

	}

	void BinaryEncoder::WriteInt8(int8 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt8(uint8 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt16(int16 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt16(uint16 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt32(int32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt32(uint32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt64(int64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt64(uint64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteFloat32(float32 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteFloat64(float64 value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteBool(bool value)
	{
		WriteBuffer((char*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteString(const std::string& value)
	{
		WriteUInt16((uint16)value.length());
		WriteBuffer(value.c_str(), (uint16)value.length());
	}

	void BinaryEncoder::WriteBuffer(const char* buffer, uint16 size)
	{
		memcpy(m_pNetworkPacket->GetBuffer() + m_pNetworkPacket->GetBufferSize(), buffer, size);
		m_pNetworkPacket->AppendBytes(size);
	}
}