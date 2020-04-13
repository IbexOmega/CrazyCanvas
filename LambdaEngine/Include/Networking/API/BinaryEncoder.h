#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API BinaryEncoder
	{
	public:
		BinaryEncoder(NetworkPacket* packet);
		~BinaryEncoder();

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
		void WriteBuffer(const char* buffer, uint16 size);

	private:
		NetworkPacket* m_pNetworkPacket;
		uint16 m_Size;
	};
}