#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include <array>

namespace LambdaEngine
{
	class IClient;
	class ServerBase;

	class LAMBDA_API NetworkDebugger
	{
	public:
		DECL_STATIC_CLASS(NetworkDebugger);

		static void RenderStatisticsWithImGUI(IClient* pClient);
		static void RenderStatisticsWithImGUI(ServerBase* pServer);


	private:
		static std::unordered_map<IClient*, std::array<float, 80>> s_PingValues;
		static uint16 s_PingValuesOffset;
		static Timestamp s_LastUpdate;
	};
}
