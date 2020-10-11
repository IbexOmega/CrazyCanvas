#pragma once

#include "Containers/String.h"

namespace LambdaEngine
{
	struct StringUtilities
	{
		template<typename T>
		static String MaskToString(T mask)
		{
			const uint32 numBytes = sizeof(mask);
			const uint32 numBits = numBytes*8;
			const uint32 numChars = numBits + numBytes - 1;
			String str((size_t)numChars, '0');
			for (uint32 bit = 0, c = 0; c < numChars; c++, bit++)
			{
				if ((bit % 8) == 0 && bit != 0 && bit != (numBits - 1))
					str[numChars - 1 - c++] = '\'';
				if (mask & (1 << bit))
					str[numChars - 1 - c] = '1';
			}

			return str;
		}
	};
}