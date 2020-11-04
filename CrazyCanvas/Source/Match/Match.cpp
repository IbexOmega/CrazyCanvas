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

void Match::KillPlayer(LambdaEngine::Entity playerEntity)
{
	using namespace LambdaEngine;

	// Get player position at time of death
	ECSCore* pECS = ECSCore::GetInstance();
	const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(playerEntity);
	const glm::vec3 position = positionComp.Position;

	// Send event to notify other systems
	PlayerDiedEvent diedEvent(playerEntity, position);
	EventQueue::SendEventImmediate(diedEvent);

	s_pMatchInstance->KillPlayer(playerEntity);
}

void Match::Tick(LambdaEngine::Timestamp deltaTime)
{
	s_pMatchInstance->Tick(deltaTime);
}

void Match::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	s_pMatchInstance->FixedTick(deltaTime);
}
