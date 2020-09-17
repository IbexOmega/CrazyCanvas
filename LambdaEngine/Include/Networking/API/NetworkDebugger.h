#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Networking/API/IPEndPoint.h"

#include <array>

namespace LambdaEngine
{
	class IClient;
	class ServerBase;

	struct ClientInfo
	{
		IClient* Client;
		std::array<float, 80> PingValues;
	};

	class LAMBDA_API NetworkDebugger
	{
	public:
		DECL_STATIC_CLASS(NetworkDebugger);

		static void RenderStatistics(IClient* pClient);
		static void RenderStatistics(ServerBase* pServer);

	private:
		static void RenderStatisticsWithImGUI();

	private:
		static std::unordered_map<IPEndPoint, ClientInfo, IPEndPointHasher> s_PingValues;
		static uint16 s_PingValuesOffset;
		static Timestamp s_LastUpdate;
	};
}
