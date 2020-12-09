#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	BinaryEncoder::BinaryEncoder(NetworkSegment* pPacket) :
		m_pNetworkPacket(pPacket)
	{

	}

	BinaryEncoder::~BinaryEncoder()
	{

	}

	bool BinaryEncoder::WriteInt8(int8 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteUInt8(uint8 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteInt16(int16 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteUInt16(uint16 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteInt32(int32 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteUInt32(uint32 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteInt64(int64 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteUInt64(uint64 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteFloat32(float32 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteFloat64(float64 value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteBool(bool value)
	{
		return WriteBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryEncoder::WriteString(const std::string& value)
	{
		if (!WriteUInt16((uint16)value.length()))
			return false;

		return WriteBuffer((const uint8*)value.c_str(), (uint16)value.length());
	}

	bool BinaryEncoder::WriteBuffer(const uint8* pBuffer, uint16 size)
	{
		return m_pNetworkPacket->Write(pBuffer, size);
	}

	bool BinaryEncoder::WriteVec2(const glm::vec2& value)
	{
		return WriteBuffer((const uint8*)&value.data, sizeof(glm::vec2));
	}

	bool BinaryEncoder::WriteVec3(const glm::vec3& value)
	{
		return WriteBuffer((const uint8*)&value.data, sizeof(glm::vec3));
	}

	bool BinaryEncoder::WriteVec4(const glm::vec4& value)
	{
		return WriteBuffer((const uint8*)&value.data, sizeof(glm::vec4));
	}

	bool BinaryEncoder::WriteQuat(const glm::quat& value)
	{
		return WriteBuffer((const uint8*)&value.data, sizeof(glm::quat));
	}
}