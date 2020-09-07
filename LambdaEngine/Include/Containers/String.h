#pragma once
#include <string>

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	using String = std::string;
}