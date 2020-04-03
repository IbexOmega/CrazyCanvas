#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <atomic>

#define MAXIMUM_PACKET_SIZE 1024

namespace LambdaEngine
{
	enum EPacketType
	{
		PACKET_TYPE_UNDEFINED = 0,
		PACKET_TYPE_USER_DATA = 1,
	};

	class LAMBDA_API NetworkPacket
	{
	public:
		NetworkPacket(EPacketType packetType);

		void WriteInt8(int8 value);
		void WriteInt16(int16 value);
		void WriteInt32(int32 value);
		void WriteInt64(int64 value);
		void WriteFloat32(float32 value);
		void WriteFloat64(float64 value);
		void WriteBool(bool value);
		void WriteString(const std::string& value);
		void WriteBuffer(const char* buffer, int16 size);

		void ReadInt8(int8& value);
		void ReadInt16(int16& value);
		void ReadInt32(int32& value);
		void ReadInt64(int64& value);
		void ReadFloat32(float32& value);
		void ReadFloat64(float64& value);
		void ReadBool(bool& value);
		void ReadString(std::string& value);
		void ReadBuffer(char* buffer, int16 bytesToRead);

		void ResetHead();
		int16 GetSize() const;
		char* GetBuffer();
		void Pack();
		void UnPack();

	private:
		char m_Buffer[MAXIMUM_PACKET_SIZE];
		int16 m_Size;
		int16 m_Head;
	};
}
