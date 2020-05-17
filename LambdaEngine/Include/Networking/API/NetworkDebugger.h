#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IClientUDP;

	class LAMBDA_API NetworkDebugger
	{
	public:
		DECL_STATIC_CLASS(NetworkDebugger);

		static void RenderStatisticsWithImGUI(IClientUDP* pClient);
	};
}
