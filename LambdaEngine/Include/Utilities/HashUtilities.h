#pragma once
#include "LambdaEngine.h"

#include <functional>

namespace LambdaEngine
{
	template<typename T>
	inline void HashCombine(size_t& hash, const T& value)
	{
		std::hash<T> hasher;
		hash ^= hasher(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
}