#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

#define MAXIMUM_SEGMENT_SIZE 1024

namespace LambdaEngine
{
	class LAMBDA_API NetworkSegment
	{
		friend class PacketTranscoder;
		friend class SegmentPool;
		friend class PacketManager;
		friend class PacketManager2;

	public:
#pragma pack(push, 1)
		struct Header
		{
			uint16 Size = 0;
			uint16 Type = 0;
			uint32 UID = 0;
			uint32 ReliableUID = 0;
		};
#pragma pack(pop)

		enum Type : uint16
		{
			TYPE_UNDEFINED				= UINT16_MAX - 0,
			TYPE_PING					= UINT16_MAX - 1,
			TYPE_SERVER_FULL			= UINT16_MAX - 2,
			TYPE_SERVER_NOT_ACCEPTING	= UINT16_MAX - 3,
			TYPE_CONNNECT				= UINT16_MAX - 4,
			TYPE_DISCONNECT				= UINT16_MAX - 5,
			TYPE_CHALLENGE				= UINT16_MAX - 6,
			TYPE_ACCEPTED				= UINT16_MAX - 7,
			TYPE_NETWORK_ACK			= UINT16_MAX - 8,
			TYPE_NETWORK_DISCOVERY		= UINT16_MAX - 9,
			TYPE_BROADCAST				= UINT16_MAX - 10,
		};

	public:
		~NetworkSegment();

		NetworkSegment* SetType(uint16 type);
		uint16 GetType() const;

		char* GetBuffer();
		const char* GetBufferReadOnly() const;
		uint16 GetBufferSize() const;

		Header& GetHeader();
		uint8 GetHeaderSize() const;

		uint16 GetTotalSize() const;

		NetworkSegment* AppendBytes(uint16 bytes);

		uint64 GetRemoteSalt() const;

		bool IsReliable() const;
		uint32 GetReliableUID() const;

		std::string ToString() const;

		void DeepCopy(NetworkSegment* pSegment) const;

	private:
		NetworkSegment();

	private:
		static void PacketTypeToString(uint16 type, std::string& str);

	private:
#ifndef LAMBDA_CONFIG_PRODUCTION
		std::string m_Borrower;
		std::string m_Type;
#endif

		Header m_Header;
		uint64 m_Salt;
		uint16 m_SizeOfBuffer;
		bool m_IsBorrowed;
		char m_pBuffer[MAXIMUM_SEGMENT_SIZE];
	};

	struct NetworkSegmentReliableUIDOrder
	{
		bool operator()(const NetworkSegment& lhs, const NetworkSegment& rhs) const
		{
			return lhs.GetReliableUID() < rhs.GetReliableUID();
		}

		bool operator()(const NetworkSegment* lhs, const NetworkSegment* rhs) const
		{
			return lhs->GetReliableUID() < rhs->GetReliableUID();
		}
	};
}