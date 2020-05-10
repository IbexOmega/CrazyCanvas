#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class LAMBDA_API NetworkChallenge
	{
	public:
		DECL_STATIC_CLASS(NetworkChallenge);

		static uint64 Compute(uint64 clientSalt, uint64 serverSalt);
	};
}