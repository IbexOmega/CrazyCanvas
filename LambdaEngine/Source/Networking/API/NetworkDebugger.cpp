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
	THashTable<IPEndPoint, ClientInfo, IPEndPointHasher> NetworkDebugger::s_ClientInfos;
	THashTable<uint16, String> NetworkDebugger::s_PacketNames;
	THashTable<uint64, String> NetworkDebugger::s_ClientNames;
	uint16 NetworkDebugger::s_PingValuesOffset = 0;
	Timestamp NetworkDebugger::s_LastUpdate = 0;
	SpinLock NetworkDebugger::s_Lock;

	void NetworkDebugger::Init()
	{
		RegisterPacketName(NetworkSegment::TYPE_UNDEFINED,				"CORE_UNDEFINED");
		RegisterPacketName(NetworkSegment::TYPE_PING,					"CORE_PING");
		RegisterPacketName(NetworkSegment::TYPE_SERVER_FULL,			"CORE_SERVER_FULL");
		RegisterPacketName(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING,	"CORE_SERVER_NOT_ACCEPTING");
		RegisterPacketName(NetworkSegment::TYPE_CONNNECT,				"CORE_CONNNECT");
		RegisterPacketName(NetworkSegment::TYPE_DISCONNECT,				"CORE_DISCONNECT");
		RegisterPacketName(NetworkSegment::TYPE_CHALLENGE,				"CORE_CHALLENGE");
		RegisterPacketName(NetworkSegment::TYPE_ACCEPTED,				"CORE_ACCEPTED");
		RegisterPacketName(NetworkSegment::TYPE_NETWORK_ACK,			"CORE_NETWORK_ACK");
		RegisterPacketName(NetworkSegment::TYPE_NETWORK_DISCOVERY,		"CORE_NETWORK_DISCOVERY");
	}

	void NetworkDebugger::RegisterPacketName(uint16 type, const String& name)
	{
		s_PacketNames.insert({ type, name });
	}

	void NetworkDebugger::RegisterClientName(IClient* pClient, const String& name)
	{
		s_ClientNames[pClient->GetUID()] = name;
	}

	void NetworkDebugger::RenderStatistics(ServerBase* pServer)
	{
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (auto& pair : pServer->GetClients())
			{
				IClient* pClient = pair.second;
				auto pIterator = s_ClientInfos.find(pair.first);
				if (pIterator == s_ClientInfos.end())
				{
					s_ClientInfos.insert({ pair.first, { pClient } });
					s_ClientNames.insert({ pClient->GetUID(), std::to_string(pClient->GetUID()) });
				}
			}

			TArray<IPEndPoint> clientsToRemove;

			for (auto& pair : s_ClientInfos)
			{
				auto pIterator = pServer->GetClients().find(pair.first);
				if (pIterator == pServer->GetClients().end())
				{
					clientsToRemove.PushBack(pair.first);
				}
			}

			for (IPEndPoint& endpoints : clientsToRemove)
			{
				s_ClientInfos.erase(endpoints);
			}
		}

		RenderStatisticsWithImGUI();
	}

	void NetworkDebugger::RenderStatistics(IClient* pClient)
	{
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			if (pClient != nullptr)
			{
				auto pIterator = s_ClientInfos.find(pClient->GetEndPoint());
				if (pIterator == s_ClientInfos.end())
				{
					s_ClientInfos.insert({ pClient->GetEndPoint(), { pClient } });
					s_ClientNames.insert({ pClient->GetUID(), std::to_string(pClient->GetUID()) });
				}
			}
			else
			{
				s_ClientInfos.clear();
			}
		}	

		RenderStatisticsWithImGUI();
	}

	void NetworkDebugger::RenderStatisticsWithImGUI()
	{
		if (EngineLoop::GetTimeSinceStart() - s_LastUpdate >= EngineLoop::GetFixedTimestep())
		{
			s_PingValuesOffset = (s_PingValuesOffset + 1) % 80;
			s_LastUpdate = EngineLoop::GetTimeSinceStart();
		}

		ImGuiRenderer::Get().DrawUI([]()
		{
			ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Network Statistics", NULL))
			{
				ImGui::Columns((int)s_ClientInfos.size() + 1, "ClientColumns");
				ImGui::Text("Client Info"); ImGui::NextColumn();

				for (auto& pair : s_ClientInfos)
				{
					ImGui::Text(s_ClientNames[pair.second.Client->GetUID()].c_str());
					ImGui::NextColumn();
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

				for (auto& pair : s_ClientInfos)
				{
					ClientInfo& clientInfo = pair.second;
					IClient* pClient = clientInfo.Client;
					uint32 color = IClient::StateToColor(pClient->GetState());
					PacketManagerBase* pManager = pClient->GetPacketManager();
					SegmentPool* pSegmentPool = pManager->GetSegmentPool();
					NetworkStatistics* pStatistics = pClient->GetStatistics();

					ImGui::NextColumn();
					ImGui::PushStyleColor(ImGuiCol_Text, UINT32_TO_IMVEC4(((uint8*)&color)));
					ImGui::TextUnformatted(IClient::StateToString(pClient->GetState()).c_str());
					ImGui::PopStyleColor();
					ImGui::Text("%d", pStatistics->GetPacketsSent());
					ImGui::Text("%d", pStatistics->GetSegmentsSent());
					ImGui::Text("%d", pStatistics->GetReliableSegmentsSent());
					ImGui::Text("%d", pStatistics->GetPacketsReceived());
					ImGui::Text("%d", pStatistics->GetSegmentsReceived());
					ImGui::Text("%d", pStatistics->GetSendingPacketLoss());
					ImGui::Text("%d", pStatistics->GetReceivingPacketLoss());
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

					clientInfo.PingValues[s_PingValuesOffset] = (float32)pStatistics->GetPing();
					ImGui::PlotLines("", clientInfo.PingValues.data(), (int)clientInfo.PingValues.size(), s_PingValuesOffset, "", 0.0f, 50.0f, ImVec2(0, 80.0f));
				
					ImGui::NewLine();

					if (ImGui::TreeNode("Segment types sent"))
					{
						const THashTable<uint16, uint32>& sentTable = pStatistics->BeginGetSentSegmentTypeCountTable();
						for (auto& p : sentTable)
						{
							ImGui::Text("%d : %s", p.second, s_PacketNames[p.first].c_str());
						}
						pStatistics->EndGetSentSegmentTypeCountTable();
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Segment types received"))
					{
						const THashTable<uint16, uint32>& sentTable = pStatistics->BeginGetReceivedSegmentTypeCountTable();
						for (auto& p : sentTable)
						{
							ImGui::Text("%d : %s", p.second, s_PacketNames[p.first].c_str());
						}
						pStatistics->EndGetReceivedSegmentTypeCountTable();
						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Packets sent size"))
					{
						uint16 samples = (uint16)clientInfo.SendSizes.size();
						uint16 value = 0;
						CCBuffer<uint16, 60>& sentHistory = pStatistics->BeginGetBytesSentHistory();
						while (sentHistory.Read(value))
						{
							clientInfo.SendSizes[clientInfo.SendSizesOffset] = (float32)value;
							clientInfo.SendSizesOffset = (clientInfo.SendSizesOffset + 1) % samples;
						}
						pStatistics->EndGetBytesSentHistory();
						ImGui::PlotHistogram("", clientInfo.SendSizes.data(), samples, clientInfo.SendSizesOffset, "", 0.0f, 255.0f, ImVec2(0, 80.0f));
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Packets received size"))
					{
						uint16 samples = (uint16)clientInfo.ReceiveSizes.size();
						uint16 value = 0;
						CCBuffer<uint16, 60>& receivedHistory = pStatistics->BeginGetBytesReceivedHistory();
						while (receivedHistory.Read(value))
						{
							clientInfo.ReceiveSizes[clientInfo.ReceiveSizesOffset] = (float32)value;
							clientInfo.ReceiveSizesOffset = (clientInfo.ReceiveSizesOffset + 1) % samples;
						}
						pStatistics->EndGetBytesReceivedHistory();
						ImGui::PlotHistogram("", clientInfo.ReceiveSizes.data(), samples, clientInfo.ReceiveSizesOffset, "", 0.0f, 255.0f, ImVec2(0, 80.0f));
						ImGui::TreePop();
					}
				}
			}
			ImGui::End();
		});
	}
}