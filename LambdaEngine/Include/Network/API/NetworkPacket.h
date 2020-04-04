#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <atomic>

#define MAXIMUM_PACKET_SIZE 1024
#define PACKET_SIZE uint16
#define PACKET_TYPE uint16

namespace LambdaEngine
{
	enum EPacketType : PACKET_TYPE
	{
		PACKET_TYPE_UNDEFINED	= 0,
		PACKET_TYPE_PING		= 1,
		PACKET_TYPE_SERVER_FULL = 2,
		PACKET_TYPE_USER_DATA	= 3,
	};

	class LAMBDA_API NetworkPacket
	{
	public:
		NetworkPacket(PACKET_TYPE packetType, bool autoDelete = true);

		void WriteInt8(int8 value);
		void WriteUInt8(uint8 value);
		void WriteInt16(int16 value);
		void WriteUInt16(uint16 value);
		void WriteInt32(int32 value);
		void WriteUInt32(uint32 value);
		void WriteInt64(int64 value);
		void WriteUInt64(uint64 value);
		void WriteFloat32(float32 value);
		void WriteFloat64(float64 value);
		void WriteBool(bool value);
		void WriteString(const std::string& value);
		void WriteBuffer(const char* buffer, PACKET_SIZE size);

		void ReadInt8(int8& value);
		void ReadUInt8(uint8& value);
		void ReadInt16(int16& value);
		void ReadUInt16(uint16& value);
		void ReadInt32(int32& value);
		void ReadUInt32(uint32& value);
		void ReadInt64(int64& value);
		void ReadUInt64(uint64& value);
		void ReadFloat32(float32& value);
		void ReadFloat64(float64& value);
		void ReadBool(bool& value);
		void ReadString(std::string& value);
		void ReadBuffer(char* buffer, PACKET_SIZE bytesToRead);

		void Reset();
		PACKET_SIZE GetSize() const;
		char* GetBuffer();
		void Pack();
		void UnPack();
		PACKET_TYPE ReadPacketType() const;
		bool ShouldAutoDelete() const;

	private:
		char m_Buffer[MAXIMUM_PACKET_SIZE];
		PACKET_SIZE m_Size;
		PACKET_SIZE m_Head;
		bool m_AutoDelete;
	};
}