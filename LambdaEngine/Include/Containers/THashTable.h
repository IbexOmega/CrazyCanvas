#pragma once
#include <unordered_map>

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	template <typename Key, typename Type, typename Hasher = std::hash<Key>>
	using THashTable = std::unordered_map<Key, Type, Hasher>;
}