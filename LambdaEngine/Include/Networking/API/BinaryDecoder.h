#pragma once

#include "LambdaEngine.h"

#include "Containers/String.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API BinaryDecoder
	{
	public:
		BinaryDecoder(NetworkSegment* pPacket);
		~BinaryDecoder();

		bool ReadInt8(int8& value);
		bool ReadUInt8(uint8& value);
		bool ReadInt16(int16& value);
		bool ReadUInt16(uint16& value);
		bool ReadInt32(int32& value);
		bool ReadUInt32(uint32& value);
		bool ReadInt64(int64& value);
		bool ReadUInt64(uint64& value);
		bool ReadFloat32(float32& value);
		bool ReadFloat64(float64& value);
		bool ReadBool(bool& value);
		bool ReadString(std::string& value);
		bool ReadBuffer(uint8* pBuffer, uint16 bytesToRead);

		bool ReadVec2(glm::vec2& value);
		bool ReadVec3(glm::vec3& value);
		bool ReadVec4(glm::vec4& value);

		bool ReadQuat(glm::quat& value);

		NetworkSegment* GetPacket();

	private:
		NetworkSegment* m_pNetworkPacket;
	};
}