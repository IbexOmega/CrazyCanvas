#include "Networking/API/NetworkDebugger.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/PacketManager.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketPool.h"

#include "Engine/EngineLoop.h"

#include <imgui.h>

namespace LambdaEngine
{
	void NetworkDebugger::RenderStatisticsWithImGUI(IClientUDP* pClient)
	{
		PacketManager* pManager = pClient->GetPacketManager();
		PacketPool* pPacketPool = pManager->GetPacketPool();
		const NetworkStatistics* pStatistics = pManager->GetStatistics();

		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Network Statistics", NULL))
		{
			ImGui::Text("Packets Sent           %d", pStatistics->GetPacketsSent());
			ImGui::Text("Messages Sent          %d", pStatistics->GetMessagesSent());
			ImGui::Text("Reliable Messages Sent %d", pStatistics->GetReliableMessagesSent());
			ImGui::Text("Packets Received       %d", pStatistics->GetPacketsReceived());
			ImGui::Text("Messages Received      %d", pStatistics->GetMessagesReceived());
			ImGui::Text("Packets Lost           %d", pStatistics->GetPacketsLost());
			ImGui::Text("Packet Loss Rate       %.1f%%", pStatistics->GetPacketLossRate() * 100.0f);
			ImGui::Text("Bytes Sent             %d", pStatistics->GetBytesSent());
			ImGui::Text("Bytes Received         %d", pStatistics->GetBytesReceived());
			ImGui::Text("Ping                   %.1f ms", pStatistics->GetPing().AsMilliSeconds());
			ImGui::Text("Local Salt             %lu", pStatistics->GetSalt());
			ImGui::Text("Remote Salt            %lu", pStatistics->GetRemoteSalt());
			ImGui::Text("Last Packet Sent       %d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestapLastSent()).AsSeconds());
			ImGui::Text("Last Packet Received   %d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestapLastReceived()).AsSeconds());

			ImGui::NewLine();

			ImGui::Text("Total Packets           %d", pPacketPool->GetSize());
			ImGui::Text("Free Packets            %d", pPacketPool->GetFreePackets());

			ImGui::End();
		}
	}
}