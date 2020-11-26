#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Networking/API/IPEndPoint.h"

#include "Containers/THashTable.h"

#include <array>

namespace LambdaEngine
{
	class IClient;
	class ServerBase;

	struct ClientInfo
	{
		IClient* Client;

		std::array<float32, 80> PingValues;

		std::array<float32, 60> SendSizes;
		std::array<float32, 60> ReceiveSizes;
		uint16 SendSizesOffset		= 0;
		uint16 ReceiveSizesOffset	= 0;
	};

	class LAMBDA_API NetworkDebugger
	{
		friend class NetworkUtils;

	public:
		DECL_STATIC_CLASS(NetworkDebugger);

		static void RenderStatistics(IClient* pClient);
		static void RenderStatistics(ServerBase* pServer);
		static void RegisterPacketName(uint16 type, const String& name);
		static void RegisterClientName(IClient* pClient, const String& name);

	private:
		static void Init();
		static void RenderStatisticsWithImGUI();

	private:
		static THashTable<IPEndPoint, ClientInfo, IPEndPointHasher> s_ClientInfos;
		static THashTable<uint16, String> s_PacketNames;
		static THashTable<uint64, String> s_ClientNames;
		static uint16 s_PingValuesOffset;
		static Timestamp s_LastUpdate;
		static SpinLock s_Lock;
	};
}
