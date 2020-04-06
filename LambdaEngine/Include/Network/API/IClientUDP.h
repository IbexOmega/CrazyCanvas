#pragma once
#include "IClient.h"

namespace LambdaEngine
{
	class LAMBDA_API IClientUDP : public IClient
	{
	public:
		DECL_INTERFACE(IClientUDP);
	};
}
