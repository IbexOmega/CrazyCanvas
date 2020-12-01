#include "Match/Match.h"
#include "Match/MatchServer.h"
#include "Match/MatchClient.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

bool Match::Init()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);

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

	std::scoped_lock<SpinLock> lock(m_Lock);
	SAFEDELETE(s_pMatchInstance);

	return true;
}

bool Match::CreateMatch(const MatchDescription* pDesc)
{
	using namespace LambdaEngine;
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (s_pMatchInstance && !s_pMatchInstance->Init(pDesc))
		{
			return false;
		}
	}

	EventQueue::SendEvent<MatchInitializedEvent>(MatchInitializedEvent(pDesc->GameMode));

	return true;
}

bool Match::ResetMatch()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->ResetMatch();
	return false;
}

void Match::StartMatch()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->MatchStart();
}

void Match::BeginLoading()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->BeginLoading();
}

void Match::KillPlaneCallback(LambdaEngine::Entity killPlaneEntity, LambdaEngine::Entity otherEntity)
{
	using namespace LambdaEngine;

	if (s_pMatchInstance != nullptr)
	{
		s_pMatchInstance->KillPlaneCallback(killPlaneEntity, otherEntity);
	}
	else
	{
		ECSCore* pECS = ECSCore::GetInstance();
		pECS->RemoveEntity(otherEntity);
	}
}

void Match::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->Tick(deltaTime);
}

void Match::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if(s_pMatchInstance)
		s_pMatchInstance->FixedTick(deltaTime);
}