#pragma once
#include <Stack>

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	template<typename T>
	using TStack = std::stack<T>;
}

