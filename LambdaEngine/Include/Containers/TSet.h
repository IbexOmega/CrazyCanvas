#pragma once
#include <set>

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
	using TSet = std::set<Key, Compare, Allocator>;
}

