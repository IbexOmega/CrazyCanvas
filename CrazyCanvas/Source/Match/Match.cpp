#include "Match/Match.h"
#include "Match/MatchServer.h"
#include "Match/MatchClient.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

bool Match::Init()
{
	using namespace LambdaEngine;

	if (MultiplayerUtils::IsServer())
	{
		s_pMatchInstance = DBG_NEW MatchServer();
	}
	else
	{
		s_pMatchInstance = DBG_NEW MatchClient();
	}

	return true;
}

bool Match::Release()
{
	using namespace LambdaEngine;

	SAFEDELETE(s_pMatchInstance);

	return true;
}

bool Match::CreateMatch(const MatchDescription* pDesc)
{
	using namespace LambdaEngine;

	if (!s_pMatchInstance->Init(pDesc))
	{
		return false;
	}

	return true;
}

bool Match::ResetMatch()
{
	s_pMatchInstance->ResetMatch();
	return false;
}

bool Match::ReleaseMatch()
{
	return false;
}

void Match::StartMatch()
{
	s_pMatchInstance->MatchStart();
}

void Match::BeginLoading()
{
	s_pMatchInstance->BeginLoading();
}

void Match::Tick(LambdaEngine::Timestamp deltaTime)
{
	s_pMatchInstance->Tick(deltaTime);
}

void Match::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	s_pMatchInstance->FixedTick(deltaTime);
}
