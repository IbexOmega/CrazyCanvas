#include "Networking/API/BinaryDecoder.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	BinaryDecoder::BinaryDecoder(const NetworkSegment* packet) :
		m_pNetworkPacket(packet),
		m_ReadHead(0)
	{

	}

	BinaryDecoder::~BinaryDecoder()
	{

	}

	void BinaryDecoder::ReadInt8(int8& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt8(uint8& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt16(int16& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt16(uint16& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt32(int32& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt32(uint32& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt64(int64& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt64(uint64& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadFloat32(float32& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadFloat64(float64& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadBool(bool& value)
	{
		ReadBuffer((uint8*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadString(std::string& value)
	{
		uint16 length;
		ReadUInt16(length);
		value.resize(length);
		ReadBuffer((uint8*)value.data(), length);
	}

	void BinaryDecoder::ReadBuffer(uint8* buffer, uint16 bytesToRead)
	{
		//If this assert is triggerd then you are reading more data than what exists in the Packet
		ASSERT(m_ReadHead + bytesToRead <= m_pNetworkPacket->GetBufferSize());

		memcpy(buffer, m_pNetworkPacket->GetBufferReadOnly() + m_ReadHead, bytesToRead);
		m_ReadHead += bytesToRead;
	}

	void BinaryDecoder::ReadVec2(glm::vec2& value)
	{
		ReadFloat32(value.x);
		ReadFloat32(value.y);
	}

	void BinaryDecoder::ReadVec3(glm::vec3& value)
	{
		ReadFloat32(value.x);
		ReadFloat32(value.y);
		ReadFloat32(value.z);
	}

	void BinaryDecoder::ReadVec4(glm::vec4& value)
	{
		ReadFloat32(value.x);
		ReadFloat32(value.y);
		ReadFloat32(value.z);
		ReadFloat32(value.w);
	}

	int8 BinaryDecoder::ReadInt8()
	{
		int8 value = 0;
		ReadInt8(value);
		return value;
	}

	uint8 BinaryDecoder::ReadUInt8()
	{
		uint8 value = 0;
		ReadUInt8(value);
		return value;
	}

	int16 BinaryDecoder::ReadInt16()
	{
		int16 value = 0;
		ReadInt16(value);
		return value;
	}

	uint16 BinaryDecoder::ReadUInt16()
	{
		uint16 value = 0;
		ReadUInt16(value);
		return value;
	}

	int32 BinaryDecoder::ReadInt32()
	{
		int32 value = 0;
		ReadInt32(value);
		return value;
	}

	uint32 BinaryDecoder::ReadUInt32()
	{
		uint32 value = 0;
		ReadUInt32(value);
		return value;
	}

	int64 BinaryDecoder::ReadInt64()
	{
		int64 value = 0;
		ReadInt64(value);
		return value;
	}

	uint64 BinaryDecoder::ReadUInt64()
	{
		uint64 value = 0;
		ReadUInt64(value);
		return value;
	}

	float32 BinaryDecoder::ReadFloat32()
	{
		float32 value = 0.0f;
		ReadFloat32(value);
		return value;
	}

	float64 BinaryDecoder::ReadFloat64()
	{
		float64 value = 0.0f;
		ReadFloat64(value);
		return value;
	}

	bool BinaryDecoder::ReadBool()
	{
		bool value = false;
		ReadBool(value);
		return value;
	}

	std::string BinaryDecoder::ReadString()
	{
		std::string value;
		ReadString(value);
		return value;
	}

	glm::vec2 BinaryDecoder::ReadVec2()
	{
		glm::vec2 value;
		ReadVec2(value);
		return value;
	}

	glm::vec3 BinaryDecoder::ReadVec3()
	{
		glm::vec3 value;
		ReadVec3(value);
		return value;
	}

	glm::vec4 BinaryDecoder::ReadVec4()
	{
		glm::vec4 value;
		ReadVec4(value);
		return value;
	}
}