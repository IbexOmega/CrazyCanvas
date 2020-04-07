#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>
#include <set>

#include "INetworkDiscoverySearcherHandler.h"
#include "Network/API/UDP/IClientUDPHandler.h"

#include "Time/API/Timestamp.h"

#include "Threading/SpinLock.h"

namespace LambdaEngine
{
	class IClientUDP;

	class LAMBDA_API NetworkDiscoverySearcher : protected IClientUDPHandler
	{
		friend class NetworkUtils;

	public:
		NetworkDiscoverySearcher(INetworkDiscoverySearcherHandler* pHandler, const std::string& uid);
		~NetworkDiscoverySearcher();
		
	protected:
		void BroadcastPacket();

		virtual void OnClientPacketReceivedUDP(IClientUDP* client, NetworkPacket* packet) override;
		virtual void OnClientErrorUDP(IClientUDP* client) override;
		virtual void OnClientStoppedUDP(IClientUDP* client) override;

	private:
		static void InitStatic();
		static void TickStatic(Timestamp dt);

	private:
		INetworkDiscoverySearcherHandler* m_pHandler;
		IClientUDP* m_pClient;
		std::string m_UID;
		NetworkPacket m_Packet;

	private:
		static std::set<NetworkDiscoverySearcher*> s_Searchers;
		static uint64 s_Timer;
		static SpinLock m_Lock;
	};
}
