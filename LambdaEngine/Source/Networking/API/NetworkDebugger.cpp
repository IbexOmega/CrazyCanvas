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

	std::unordered_map<IPEndPoint, ClientInfo, IPEndPointHasher> NetworkDebugger::s_PingValues;
	uint16 NetworkDebugger::s_PingValuesOffset = 0;
	Timestamp NetworkDebugger::s_LastUpdate = 0;

	void NetworkDebugger::RenderStatistics(ServerBase* pServer)
	{
		for (auto& pair : pServer->GetClients())
		{
			auto pIterator = s_PingValues.find(pair.first);
			if (pIterator == s_PingValues.end())
			{
				s_PingValues.insert({ pair.first, { pair.second, std::array<float, 80>() } });
			}			
		}	

		TArray<IPEndPoint> clientsToRemove;

		for (auto& pair : s_PingValues)
		{
			auto pIterator = pServer->GetClients().find(pair.first);
			if (pIterator == pServer->GetClients().end())
			{
				clientsToRemove.PushBack(pair.first);
			}			
		}

		for (IPEndPoint& endpoints : clientsToRemove)
		{
			s_PingValues.erase(endpoints);
		}

		RenderStatisticsWithImGUI();
	}

	void NetworkDebugger::RenderStatistics(IClient* pClient)
	{
		if (pClient != nullptr)
		{
			auto pIterator = s_PingValues.find(pClient->GetEndPoint());
			if (pIterator == s_PingValues.end())
			{
				s_PingValues.insert({ pClient->GetEndPoint(), { pClient, std::array<float, 80>() } });
			}
		}
		else
		{
			s_PingValues.clear();
		}

		RenderStatisticsWithImGUI();
	}

	void NetworkDebugger::RenderStatisticsWithImGUI()
	{
		if (EngineLoop::GetTimeSinceStart() - s_LastUpdate >= EngineLoop::GetFixedTimestep()/*Timestamp::MilliSeconds(100)*/)
		{
			s_PingValuesOffset = (s_PingValuesOffset + 1) % 80;
			s_LastUpdate = EngineLoop::GetTimeSinceStart();
		}

		ImGuiRenderer::Get().DrawUI([]()
		{
			ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Network Statistics", NULL))
			{
				ImGui::Columns((int)s_PingValues.size() + 1, "ClientColumns");
				ImGui::Text("Client Info"); ImGui::NextColumn();

				for (int i = 0; i < s_PingValues.size(); i++)
				{
					ImGui::Text("Client %d", i); ImGui::NextColumn();
				}

				ImGui::Text("State");
				ImGui::Text("Packets Sent");
				ImGui::Text("Segments Sent");
				ImGui::Text("Reliable Segments Sent");
				ImGui::Text("Packets Received");
				ImGui::Text("Segments Received");
				ImGui::Text("Packets Lost (S)");
				ImGui::Text("Packets Lost (R)");
				ImGui::Text("Packet Loss Rate (S)");
				ImGui::Text("Packet Loss Rate (R)");
				ImGui::Text("Bytes Sent");
				ImGui::Text("Bytes Received");
				ImGui::Text("Local Salt");
				ImGui::Text("Remote Salt");
				ImGui::Text("Last Packet Sent");
				ImGui::Text("Last Packet Received");
				ImGui::Text("Segments Resent");
				ImGui::Text("Total Segments");
				ImGui::Text("Free Segments");
				ImGui::Text("Ping");
				ImGui::NewLine();
				ImGui::NewLine();
				ImGui::Text("Live Ping");

				for (auto& pair : s_PingValues)
				{
					IClient* pClient = pair.second.Client;
					uint32 color = IClient::StateToColor(pClient->GetState());
					PacketManagerBase* pManager = pClient->GetPacketManager();
					SegmentPool* pSegmentPool = pManager->GetSegmentPool();
					const NetworkStatistics* pStatistics = pClient->GetStatistics();

					ImGui::NextColumn();
					ImGui::PushStyleColor(ImGuiCol_Text, UINT32_TO_IMVEC4(((uint8*)&color)));
					ImGui::TextUnformatted(IClient::StateToString(pair.second.Client->GetState()).c_str());
					ImGui::PopStyleColor();
					ImGui::Text("%d", pStatistics->GetPacketsSent());
					ImGui::Text("%d", pStatistics->GetSegmentsSent());
					ImGui::Text("%d", pStatistics->GetReliableSegmentsSent());
					ImGui::Text("%d", pStatistics->GetPacketsReceived());
					ImGui::Text("%d", pStatistics->GetSegmentsReceived());
					ImGui::Text("%d", pStatistics->GetSendingPacketsLost());
					ImGui::Text("%d", pStatistics->GetReceivingPacketsLost());
					ImGui::Text("%.1f%%", pStatistics->GetSendingPacketLossRate() * 100.0f);
					ImGui::Text("%.1f%%", pStatistics->GetReceivingPacketLossRate() * 100.0f);
					ImGui::Text("%d", pStatistics->GetBytesSent());
					ImGui::Text("%d", pStatistics->GetBytesReceived());
					ImGui::Text("%llu", pStatistics->GetSalt());
					ImGui::Text("%llu", pStatistics->GetRemoteSalt());
					ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastSent()).AsSeconds());
					ImGui::Text("%d s", (int32)(EngineLoop::GetTimeSinceStart() - pStatistics->GetTimestampLastReceived()).AsSeconds());
					ImGui::Text("%d", pStatistics->GetSegmentsResent());
					ImGui::Text("%d", pSegmentPool->GetSize());
					ImGui::Text("%d", pSegmentPool->GetFreeSegments());
					ImGui::Text("%.1f ms", pStatistics->GetPing());

					pair.second.PingValues[s_PingValuesOffset] = (float32)pStatistics->GetPing();
					ImGui::PlotLines("", pair.second.PingValues.data(), (int)pair.second.PingValues.size(), s_PingValuesOffset, "", 0.0f, 50.0f, ImVec2(0, 80.0f));
				
					ClientRemoteBase* pClientRemote = dynamic_cast<ClientRemoteBase*>(pClient);
					if (pClientRemote)
					{
						ImGui::NewLine();
						std::scoped_lock<SpinLock> lock(pClientRemote->m_LockShit);
						for (auto& p : pClientRemote->packets)
						{
							ImGui::Text("Type %d: %d", p.first, p.second);
						}
					}	
				}
			}
			ImGui::End();
		});
	}
}