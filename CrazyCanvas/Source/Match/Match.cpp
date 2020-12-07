#include "Match/Match.h"
#include "Match/MatchServer.h"
#include "Match/MatchClient.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

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

	if (s_pMatchInstance)
	{
		if (!ResetMatch())
		{
			LOG_ERROR("Failed to reset s_pMatchInstance");
			return false;
		}

		if (!s_pMatchInstance->Init(pDesc))
		{
			LOG_ERROR("Failed to initialize s_pMatchInstance");
			return false;
		}
	}
	else
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (MultiplayerUtils::IsServer())
			s_pMatchInstance = DBG_NEW MatchServer();
		else
			s_pMatchInstance = DBG_NEW MatchClient();

		if (!s_pMatchInstance->Init(pDesc))
		{
			LOG_ERROR("Failed to initialize s_pMatchInstance");
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
	{
		s_pMatchInstance->ResetMatch();
		return true;
	}

	return false;
}

void Match::StartMatch()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->MatchStart();
	else
		LOG_ERROR("Match::StartMatch() Failed because s_pMatchInstance == nullptr");
}

void Match::BeginLoading()
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_Lock);
	if (s_pMatchInstance)
		s_pMatchInstance->BeginLoading();
	else
		LOG_ERROR("Match::BeginLoading() Failed because s_pMatchInstance == nullptr");
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