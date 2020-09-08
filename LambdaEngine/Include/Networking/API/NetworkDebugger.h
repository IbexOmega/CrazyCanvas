#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IClient;

	class LAMBDA_API NetworkDebugger
	{
	public:
		DECL_STATIC_CLASS(NetworkDebugger);

		static void RenderStatisticsWithImGUI(IClient* pClient);
	};
}
