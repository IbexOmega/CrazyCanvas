#include "Networking/API/NetworkDebugger.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/PacketManagerUDP.h"
#include "Networking/API/IClient.h"
#include "Networking/API/SegmentPool.h"

#include "Engine/EngineLoop.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

namespace LambdaEngine
{
	void NetworkDebugger::RenderStatisticsWithImGUI(IClient* pClient)
	{
		PacketManagerBase* pManager = pClient->GetPacketManager();
		SegmentPool* pSegmentPool = pManager->GetSegmentPool();
		const NetworkStatistics* pStatistics = pManager->GetStatistics();

		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Network Statistics", NULL))
		{
			ImGui::Text("Packets Sent           %d", pStatistics->GetPacketsSent());
			ImGui::Text("Segments Sent          %d", pStatistics->GetSegmentsSent());
			ImGui::Text("Reliable Segments Sent %d", pStatistics->GetReliableSegmentsSent());
			ImGui::Text("Packets Received       %d", pStatistics->GetPacketsReceived());
			ImGui::Text("Segments Received      %d", pStatistics->GetSegmentsReceived());
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

			ImGui::Text("Total Segments         %d", pSegmentPool->GetSize());
			ImGui::Text("Free Segments          %d", pSegmentPool->GetFreeSegments());

			ImGui::End();
		}
	}
}