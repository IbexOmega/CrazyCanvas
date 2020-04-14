#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

#define MAXIMUM_PACKET_SIZE 1024

namespace LambdaEngine
{
	class LAMBDA_API NetworkPacket
	{
		friend class PacketDispatcher;

	public:
#pragma pack(push, 1)
		struct Header
		{
			uint16 Size = 0;
			uint16 Type = 0;
			uint32 UID = 0;
		};
#pragma pack(pop)

	public:
		~NetworkPacket();

		void SetType(uint16 type);
		uint16 GetType() const;

		char* GetBuffer();
		const char* GetBufferReadOnly() const;
		uint16 GetBufferSize() const;

		Header& GetHeader();
		uint8 GetHeaderSize() const;

		uint16 GetTotalSize() const;

		void AppendBytes(uint16 bytes);

	private:
		NetworkPacket();

	private:
		uint16 m_SizeOfBuffer;
		char m_pBuffer[MAXIMUM_PACKET_SIZE];
		Header m_Header;
	};
}