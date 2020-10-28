#include "Match/Match.h"
#include "Match/MatchServer.h"
#include "Match/MatchClient.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Multiplayer/Packet/PacketType.h"

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
	return false;
}

bool Match::ReleaseMatch()
{
	return false;
}

void Match::Tick(LambdaEngine::Timestamp deltaTime)
{
	s_pMatchInstance->Tick(deltaTime);
}