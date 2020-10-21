#pragma once

/*
* Updated to C++, zedwood.com 2012
* Based on Olivier Gay's version
* See Modified BSD License below:
*
* FIPS 180-2 SHA-224/256/384/512 implementation
* Issue date:  04/30/2005
* http://www.ouah.org/ogay/sha2/
*
* Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
* All rights reserved.
*/

#include "Containers/String.h"

namespace LambdaEngine
{
#define SHA2_SHFR(x, n)		(x >> n)
#define SHA2_ROTR(x, n)		((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_ROTL(x, n)		((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)	((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z)	((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x) (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x) (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x) (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x) (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))
#define SHA2_UNPACK32(x, str)					\
{												\
	*((str) + 3) = (uint8) ((x)		 );			\
	*((str) + 2) = (uint8) ((x) >>  8);			\
	*((str) + 1) = (uint8) ((x) >> 16);			\
	*((str) + 0) = (uint8) ((x) >> 24);			\
}
#define SHA2_PACK32(str, x)						\
{												\
	*(x) =		((uint32) *((str) + 3)		)	\
			|	((uint32) *((str) + 2) <<  8)	\
			|	((uint32) *((str) + 1) << 16)	\
			|	((uint32) *((str) + 0) << 24);	\
}
	constexpr const uint32 SHA224_256_BLOCK_SIZE = (512 / 8);
	constexpr const uint32 DIGEST_SIZE = (256 / 8);

	struct SHA256Hash
	{
		union
		{
			byte Data[DIGEST_SIZE];

			struct
			{
				uint64 SHA256Chunk0;
				uint64 SHA256Chunk1;
			};
		};

		FORCEINLINE bool operator==(const SHA256Hash& other) const noexcept
		{
			return 
				SHA256Chunk0 == other.SHA256Chunk0 && 
				SHA256Chunk1 == other.SHA256Chunk1;
		}
	};

	class SHA256
	{
	public:
		void Init();
		void Update(const byte* pMessage, uint32 len);
		void Final(byte* pDigest);

	public:
		static SHA256Hash Hash(const String& input);

	private:
		void Transform(const byte* pMessage, uint32 block_nb);

	private:
		uint32	m_tot_len;
		uint32	m_len;
		uint8	m_block[2 * SHA224_256_BLOCK_SIZE];
		uint32	m_h[8];

	private:
		const static uint32 sha256_k[];
	};
}