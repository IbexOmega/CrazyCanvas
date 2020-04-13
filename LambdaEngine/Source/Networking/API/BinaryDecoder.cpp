#include "Networking/API/BinaryDecoder.h"

#include "Networking/API/NetworkPacket.h"

namespace LambdaEngine
{
	BinaryDecoder::BinaryDecoder(const NetworkPacket* packet) :
		m_pNetworkPacket(packet),
		m_ReadHead(0)
	{

	}

	BinaryDecoder::~BinaryDecoder()
	{

	}

	void BinaryDecoder::ReadInt8(int8& value)
	{
		ReadBuffer(&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt8(uint8& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt16(int16& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt16(uint16& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt32(int32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt32(uint32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadInt64(int64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadUInt64(uint64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadFloat32(float32& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadFloat64(float64& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadBool(bool& value)
	{
		ReadBuffer((char*)&value, sizeof(value));
	}

	void BinaryDecoder::ReadString(std::string& value)
	{
		int16 length;
		ReadInt16(length);
		value.resize(length);
		ReadBuffer(value.data(), length);
	}

	void BinaryDecoder::ReadBuffer(char* buffer, uint16 bytesToRead)
	{
		memcpy(buffer, m_pNetworkPacket->GetBufferReadOnly() + m_ReadHead, bytesToRead);
		m_ReadHead += bytesToRead;
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
}