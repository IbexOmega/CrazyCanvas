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
		BinaryDecoder(const NetworkSegment* packet);
		~BinaryDecoder();

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
		void ReadBuffer(uint8* pBuffer, uint16 bytesToRead);

		void ReadVec2(glm::vec2& value);
		void ReadVec3(glm::vec3& value);
		void ReadVec4(glm::vec4& value);


		int8		ReadInt8();
		uint8		ReadUInt8();
		int16		ReadInt16();
		uint16		ReadUInt16();
		int32		ReadInt32();
		uint32		ReadUInt32();
		int64		ReadInt64();
		uint64		ReadUInt64();
		float32		ReadFloat32();
		float64		ReadFloat64();
		bool		ReadBool();
		std::string ReadString();

		glm::vec2 ReadVec2();
		glm::vec3 ReadVec3();
		glm::vec4 ReadVec4();

	private:
		const NetworkSegment* m_pNetworkPacket;
		uint16 m_ReadHead;
	};
}