#pragma once
#include "Defines.h"

#ifdef MessageBox
	#undef MessageBox
#endif

namespace LambdaEngine
{
	class Misc
	{
	public:
		DECL_STATIC_CLASS(Misc);

		static void MessageBox(const char*, const char*) { }
	};
}