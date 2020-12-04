#include "Lobby/ServerManager.h"

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineLoop.h"
#include "Engine/EngineConfig.h"

#include "Events/ServerEvents.h"

#include "Lobby/ServerFileStorage.h"

#include "Multiplayer/ClientHelper.h"

#include "Math/Random.h"

using namespace LambdaEngine;

void ServerManager::Init()
{
	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(&ServerManager::OnServerResponse);

	TArray<ServerInfo> serverInfos;
	uint16 defaultPort = (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
	ServerFileStorage::LoadServers(serverInfos, defaultPort);

	for (ServerInfo& serverInfo : serverInfos)
	{
		serverInfo.ServerUID = Random::UInt64();
		AddOrUpdateServer(s_ServersWAN, serverInfo);
		ClientHelper::AddNetworkDiscoveryTarget(serverInfo.EndPoint.GetAddress());
	}
}

void ServerManager::Release()
{
	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(&ServerManager::OnServerResponse);
}

const THashTable<uint64, ServerInfo>& ServerManager::GetServersLAN()
{
	return s_ServersLAN;
}

const THashTable<uint64, ServerInfo>& ServerManager::GetServersWAN()
{
	return s_ServersWAN;
}

void ServerManager::RegisterNewServer(const ServerInfo& serverInfo)
{
	ClientHelper::AddNetworkDiscoveryTarget(serverInfo.EndPoint.GetAddress());

	for (auto& pair : s_ServersWAN)
	{
		if (pair.second.EndPoint == serverInfo.EndPoint)
		{
			return;
		}
	}

	TArray<ServerInfo> servers;
	servers.Reserve((uint32)s_ServersWAN.size());

	for (auto& pair : s_ServersWAN)
	{
		servers.PushBack(pair.second);
	}

	servers.PushBack(serverInfo);

	ServerFileStorage::SaveServers(servers);
}

void ServerManager::Tick()
{
	Timestamp currentTime = EngineLoop::GetTimeSinceStart();
	if (currentTime - s_TimestampLastCheck > s_Interval)
	{
		s_TimestampLastCheck = currentTime;
		RemoveOldServers(s_ServersLAN, currentTime);

		for (auto& pair : s_ServersWAN)
		{
			ServerInfo& serverInfo = pair.second;
			if (currentTime - serverInfo.LastPinged > s_MaxOfflineTime)
			{
				if (serverInfo.IsOnline)
				{
					serverInfo.IsOnline = false;
					ServerOfflineEvent event(pair.second);
					EventQueue::SendEventImmediate(event);
				}
			}
		}
	}
}

bool ServerManager::OnServerResponse(const ServerDiscoveredEvent& event)
{
	BinaryDecoder* pDecoder = event.pDecoder;

	ServerInfo serverInfo;
	serverInfo.Ping			= (uint16)event.Ping.AsMilliSeconds();
	serverInfo.LastPinged	= EngineLoop::GetTimeSinceStart();
	serverInfo.EndPoint		= *event.pEndPoint;
	serverInfo.ServerUID	= event.ServerUID;
	serverInfo.IsLAN		= event.IsLAN;
	serverInfo.IsOnline		= true;

	pDecoder->ReadUInt8(serverInfo.Players);
	pDecoder->ReadUInt8(serverInfo.MaxPlayers);
	pDecoder->ReadString(serverInfo.Name);
	pDecoder->ReadString(serverInfo.MapName);
	pDecoder->ReadInt32(serverInfo.ClientHostID);

	if (serverInfo.IsLAN)
		AddOrUpdateServer(s_ServersLAN, serverInfo);
	else
		AddOrUpdateServer(s_ServersWAN, serverInfo);

	return true;
}

void ServerManager::AddOrUpdateServer(LambdaEngine::THashTable<uint64, ServerInfo>& table, const ServerInfo& serverInfo)
{
	auto iterator = table.find(serverInfo.ServerUID);

	ServerInfo serverInfoOld;

	if (iterator == table.end())
	{
		bool changed = false;

		for (auto& pair : table)
		{
			if (pair.second.EndPoint == serverInfo.EndPoint)
			{
				serverInfoOld = pair.second;
				table.insert({ serverInfo.ServerUID, serverInfo });
				table.erase(pair.first);
				changed = true;
				break;
			}
		}

		if (!changed)
		{
			table.insert({ serverInfo.ServerUID, serverInfo });
			ServerOnlineEvent event(serverInfo);
			EventQueue::SendEventImmediate(event);
		}
		else
		{
			ServerUpdatedEvent event(serverInfo, serverInfoOld);
			EventQueue::SendEventImmediate(event);
		}
	}
	else
	{
		ServerInfo& currentInfo = iterator->second;
		if (currentInfo != serverInfo)
		{
			serverInfoOld = currentInfo;
			currentInfo = serverInfo;
			ServerUpdatedEvent event(currentInfo, serverInfoOld);
			EventQueue::SendEventImmediate(event);
		}
	}
}

void ServerManager::RemoveOldServers(LambdaEngine::THashTable<uint64, ServerInfo>& table, Timestamp currentTime)
{
	TArray<ServerInfo> serversToErase;
	for (auto& pair : table)
	{
		if (currentTime - pair.second.LastPinged > s_MaxOfflineTime)
		{
			serversToErase.PushBack(pair.second);
		}
	}

	for (ServerInfo& serverInfo : serversToErase)
	{
		table.erase(serverInfo.ServerUID);
		ServerOfflineEvent event(serverInfo);
		EventQueue::SendEventImmediate(event);
	}
}