#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>

namespace LambdaEngine
{
	class LAMBDA_API NetworkDiscovery
	{
	public:
		NetworkDiscovery(const std::string& uid, uint16 port);
		~NetworkDiscovery();
		
	private:

	};
}
