#include "Networking/API/BinaryDecoder.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	BinaryDecoder::BinaryDecoder(NetworkSegment* pPacket) :
		m_pNetworkPacket(pPacket)
	{

	}

	BinaryDecoder::~BinaryDecoder()
	{

	}

	bool BinaryDecoder::ReadInt8(int8& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadUInt8(uint8& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadInt16(int16& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadUInt16(uint16& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadInt32(int32& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadUInt32(uint32& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadInt64(int64& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadUInt64(uint64& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadFloat32(float32& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadFloat64(float64& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadBool(bool& value)
	{
		return ReadBuffer((uint8*)&value, sizeof(value));
	}

	bool BinaryDecoder::ReadString(std::string& value)
	{
		uint16 length;
		if (!ReadUInt16(length))
			return false;

		value.resize(length);
		return ReadBuffer((uint8*)value.data(), length);
	}

	bool BinaryDecoder::ReadBuffer(uint8* pBuffer, uint16 bytesToRead)
	{
		return m_pNetworkPacket->Read(pBuffer, bytesToRead);
	}

	bool BinaryDecoder::ReadVec2(glm::vec2& value)
	{
		return ReadBuffer((uint8*)&value.data, sizeof(glm::vec2));
	}

	bool BinaryDecoder::ReadVec3(glm::vec3& value)
	{
		return ReadBuffer((uint8*)&value.data, sizeof(glm::vec3));
	}

	bool BinaryDecoder::ReadVec4(glm::vec4& value)
	{
		return ReadBuffer((uint8*)&value.data, sizeof(glm::vec4));
	}

	bool BinaryDecoder::ReadQuat(glm::quat& value)
	{
		return ReadBuffer((uint8*)&value.data, sizeof(glm::quat));
	}

	NetworkSegment* BinaryDecoder::GetPacket()
	{
		return m_pNetworkPacket;
	}
}