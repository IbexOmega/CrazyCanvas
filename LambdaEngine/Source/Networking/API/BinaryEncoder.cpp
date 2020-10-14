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
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt8(uint8 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt16(int16 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt16(uint16 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt32(int32 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt32(uint32 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteInt64(int64 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteUInt64(uint64 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteFloat32(float32 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteFloat64(float64 value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteBool(bool value)
	{
		WriteBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryEncoder::WriteString(const std::string& value)
	{
		WriteUInt16((uint16)value.length());
		WriteBuffer((const uint8*)value.c_str(), (uint16)value.length());
	}

	void BinaryEncoder::WriteBuffer(const uint8* pBuffer, uint16 size)
	{
		memcpy(m_pNetworkPacket->GetBuffer() + m_pNetworkPacket->GetBufferSize(), pBuffer, size);
		m_pNetworkPacket->AppendBytes(size);
	}

	void BinaryEncoder::WriteVec2(const glm::vec2& value)
	{
		WriteBuffer((const uint8*)&value.data, sizeof(glm::vec2));
	}

	void BinaryEncoder::WriteVec3(const glm::vec3& value)
	{
		WriteBuffer((const uint8*)&value.data, sizeof(glm::vec3));
	}

	void BinaryEncoder::WriteVec4(const glm::vec4& value)
	{
		WriteBuffer((const uint8*)&value.data, sizeof(glm::vec4));
	}

	void BinaryEncoder::WriteQuat(const glm::quat& value)
	{
		WriteBuffer((const uint8*)&value.data, sizeof(glm::quat));
	}
}