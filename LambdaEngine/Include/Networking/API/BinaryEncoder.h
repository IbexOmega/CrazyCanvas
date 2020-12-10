#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API BinaryEncoder
	{
	public:
		BinaryEncoder(NetworkSegment* pPacket);
		~BinaryEncoder();

		bool WriteInt8(int8 value);
		bool WriteUInt8(uint8 value);
		bool WriteInt16(int16 value);
		bool WriteUInt16(uint16 value);
		bool WriteInt32(int32 value);
		bool WriteUInt32(uint32 value);
		bool WriteInt64(int64 value);
		bool WriteUInt64(uint64 value);
		bool WriteFloat32(float32 value);
		bool WriteFloat64(float64 value);
		bool WriteBool(bool value);
		bool WriteString(const std::string& value);
		bool WriteBuffer(const uint8* pBuffer, uint16 size);

		bool WriteVec2(const glm::vec2& value);
		bool WriteVec3(const glm::vec3& value);
		bool WriteVec4(const glm::vec4& value);

		bool WriteQuat(const glm::quat& value);

	private:
		NetworkSegment* m_pNetworkPacket;
	};
}