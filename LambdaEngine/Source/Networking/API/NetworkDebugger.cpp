#include "Networking/API/NetworkDebugger.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/PacketManagerBase.h"
#include "Networking/API/IClient.h"
#include "Networking/API/ServerBase.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/ClientRemoteBase.h"

#include "Rendering/ImGuiRenderer.h"

#include "Engine/EngineLoop.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

#define UINT32_TO_IMVEC4(c) ImVec4(c[0] / 255.0F, c[1] / 255.0F, c[2] / 255.0F, c[3] / 255.0F)

namespace LambdaEngine
{

	std::unordered_map<IClient*, std::array<float, 80>> NetworkDebugger::s_PingValues;
	uint16 NetworkDebugger::s_PingValuesOffset = 0;
	Timestamp NetworkDebugger::s_LastUpdate = 0;

	void NetworkDebugger::RenderStatisticsWithImGUI(ServerBase* pServer)
	{
		for (auto& pair : pServer->GetClients())
		{
			auto pIterator = s_PingValues.find(pair.second);
			if (pIterator == s_PingValues.end())
			{
				s_PingValues.insert({ pair.second, std::array<float, 80>() });
				auto client = s_PingValues.find(pair.second);
				client->second.fill({ 0.0f });
			}			
		}	

		TArray<IClient*> clientsToRemove;

		for (auto& pair : s_PingValues)
		{
			auto pIterator = pServer->GetClients().find(pair.first->GetEndPoint());
			if (pIterator == pServer->GetClients().end())
			{
				clientsToRemove.PushBack(pair.first);
			}			
		}

		for (IClient* pClient: clientsToRemove)
		{
			s_PingValues.erase(pClient);
		}

		if (EngineLoop::GetTimeSinceStart() - s_LastUpdate >= Timestamp::MilliSeconds(100))
		{
			s_PingValuesOffset = (s_PingValuesOffset + 1) % 80;
			s_LastUpdate = EngineLoop::GetTimeSinceStart();
		}


		ImGuiRenderer::Get().DrawUI([pServer]()
		{
				ImGui::ShowDemoWindow();

				ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Network Statistics", NULL))
				{
					//uint32 color = IClient::StateToColor(pClient->GetState());
					// instead of 2 dynamically add columns
					ImGui::Columns(pServer->GetClientCount() + 1, "ClientColumns");
					ImGui::Text("Client Info"); ImGui::NextColumn();
					//For each client add client + number
					for (int i = 0; i < pServer->GetClientCount(); i++)
					{
						ImGui::Text("Client %d", i); ImGui::NextColumn();
					}

					ImGui::Text("State");
					ImGui::Text("Packets Sent");
					ImGui::Text("Segments Sent");
					ImGui::Text("Reliable Segments Sent");
					ImGui::Text("Packets Received");
					ImGui::Text("Segments Received");
					ImGui::Text("Packets Lost");
					ImGui::Text("Packet Loss Rate");
					ImGui::Text("Bytes Sent");
					ImGui::Text("Bytes Received");
					ImGui::Text("Local Salt");
					ImGui::Text("Remote Salt");
					ImGui::Text("Last Packet Sent");
					ImGui::Text("Last Packet Received");
					ImGui::Text("Total Segments");
					ImGui::Text("Free Segments");
					ImGui::Text("Ping");
					ImGui::NewLine();
					ImGui::NewLine();
					ImGui::Text("Live Ping");

					for (auto& pair : s_PingValues)
					{
						uint32 color = IClient::StateToColor(pair.first->GetState());
						PacketManagerBase* pManager = pair.first->GetPacketManager();
						SegmentPool* pSegmentPool = pManager->GetSegmentPool();
						const NetworkStatistics* pStatistics = pair.first->GetStatistics();

						ImGui::NextColumn();
						ImGui::PushStyleColor(ImGuiCol_Text, UINT32_TO_IMVEC4(((uint8*)&color)));
						ImGui::TextUnformatted(IClient::StateToString(pair.first->GetState()).c_str());
						ImGui::PopStyleColor();
						ImGui::Text("%d", pStatistics->GetPacketsSent());
						ImGui::Text("%d", pStatistics->GetSegmentsSent());
						ImGui::Text("%d", pStatistics->GetReliableSegmentsSent());
						ImGui::Text("%d", pStatistics->GetPacketsReceived());
						ImGui::Text("%d", pStatistics->GetSegmentsReceived());
						ImGui::Text("%d", pStatistics->GetPacketsLost());
						ImGui::Text("%.1f%%", pStatistics->GetPacketLossRate() * 100.0f);
						ImGui::Text("%d", pStatistics->GetBytesSent());
						ImGui::Text("%d", pStatistics->GetBytesReceived());
						ImGui::Text("%lu", pStatistics->GetSalt());
						ImGui::Text("%lu", pStatistics->GetRemoteSalt());
						ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastSent()).AsSeconds());
						ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastReceived()).AsSeconds());
						ImGui::Text("%d", pSegmentPool->GetSize());
						ImGui::Text("%d", pSegmentPool->GetFreeSegments());
						ImGui::Text("%.1f ms", pStatistics->GetPing().AsMilliSeconds());

						pair.second[s_PingValuesOffset] = pStatistics->GetPing().AsMilliSeconds();
						ImGui::PlotLines("", pair.second.data(), pair.second.size(), s_PingValuesOffset, "", 0.0f, 30.0f, ImVec2(0, 80.0f));
					}
					ImGui::End();
				}
		});
	}

	void NetworkDebugger::RenderStatisticsWithImGUI(IClient* pClient)
	{
		ImGuiRenderer::Get().DrawUI([pClient]()
		{
			PacketManagerBase* pManager = pClient->GetPacketManager();
			SegmentPool* pSegmentPool = pManager->GetSegmentPool();
			const NetworkStatistics* pStatistics = pClient->GetStatistics();

			ImGui::ShowDemoWindow();

			ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Network Statistics", NULL))
			{
				uint32 color = IClient::StateToColor(pClient->GetState());
				ImGui::Columns(2, "ClientColumns");
				ImGui::Separator();
				ImGui::Text("Client Info"); ImGui::NextColumn();
				ImGui::Text("Client"); ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("State");

				ImGui::Text("Packets Sent");
				ImGui::Text("Segments Sent");
				ImGui::Text("Reliable Segments Sent");
				ImGui::Text("Packets Received");
				ImGui::Text("Segments Received");
				ImGui::Text("Packets Lost");
				ImGui::Text("Packet Loss Rate");
				ImGui::Text("Bytes Sent");
				ImGui::Text("Bytes Received");
				ImGui::Text("Ping");
				ImGui::Text("Local Salt");
				ImGui::Text("Remote Salt");
				ImGui::Text("Last Packet Sent");
				ImGui::Text("Last Packet Received");
				ImGui::Text("Total Segments");
				ImGui::Text("Free Segments");
				ImGui::NewLine();
				ImGui::NewLine();
				ImGui::Text("Live Ping");

				ImGui::NextColumn();

				ImGui::PushStyleColor(ImGuiCol_Text, UINT32_TO_IMVEC4(((uint8*)&color)));
				ImGui::TextUnformatted(IClient::StateToString(pClient->GetState()).c_str());
				ImGui::PopStyleColor();
				ImGui::Text("%d", pStatistics->GetPacketsSent());
				ImGui::Text("%d", pStatistics->GetSegmentsSent());
				ImGui::Text("%d", pStatistics->GetReliableSegmentsSent());
				ImGui::Text("%d", pStatistics->GetPacketsReceived());
				ImGui::Text("%d", pStatistics->GetSegmentsReceived());
				ImGui::Text("%d", pStatistics->GetPacketsLost());
				ImGui::Text("%.1f%%", pStatistics->GetPacketLossRate() * 100.0f);
				ImGui::Text("%d", pStatistics->GetBytesSent());
				ImGui::Text("%d", pStatistics->GetBytesReceived());
				ImGui::Text("%.1f ms", pStatistics->GetPing().AsMilliSeconds());
				ImGui::Text("%lu", pStatistics->GetSalt());
				ImGui::Text("%lu", pStatistics->GetRemoteSalt());
				ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastSent()).AsSeconds());
				ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastReceived()).AsSeconds());
				ImGui::Text("%d", pSegmentPool->GetSize());
				ImGui::Text("%d", pSegmentPool->GetFreeSegments());

				ImGui::End();
			}
		});
	}
}