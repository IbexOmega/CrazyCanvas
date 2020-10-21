#include "Match/Match.h"
#include "Match/MatchServer.h"
#include "Match/MatchClient.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

bool Match::Init()
{
	using namespace LambdaEngine;

	EventQueue::RegisterEventHandler<PacketReceivedEvent>(&Match::OnPacketReceived);
	return true;
}

bool Match::Release()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<PacketReceivedEvent>(&Match::OnPacketReceived);
	return true;
}

bool Match::CreateMatch(const MatchDescription* pMatch)
{
	using namespace LambdaEngine;

	SAFEDELETE(s_pMatchInstance);

	if (MultiplayerUtils::IsServer())
	{
		s_pMatchInstance = DBG_NEW MatchServer();
	}
	else
	{
		s_pMatchInstance = DBG_NEW MatchClient();
	}

	return false;
}

bool Match::ResetMatch()
{
	return false;
}

bool Match::ReleaseMatch()
{
	return false;
}

bool Match::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	return false;
}
