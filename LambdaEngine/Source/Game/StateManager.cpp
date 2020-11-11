#include "Game/StateManager.h"

#include "ECS/ECSCore.h"
#include "Game/State.h"

namespace LambdaEngine
{
	StateManager StateManager::s_Instance;

	StateManager::StateManager()
		:m_pEnqueuedState(nullptr),
		m_EnqueuedTransitionAction(STATE_TRANSITION::PUSH)
	{}

	StateManager::~StateManager()
	{
	}

	bool StateManager::Init(ECSCore* pECS)
	{
		m_pECS = pECS;
		return m_pECS;
	}

	bool StateManager::Release()
	{
		while (!m_StatesToDelete.empty())
		{
			SAFEDELETE(m_StatesToDelete.front());
			m_StatesToDelete.pop();
		}

		while (!m_States.empty())
		{
			SAFEDELETE(m_States.top());
			m_States.pop();
		}

		return true;
	}

	void StateManager::EnqueueStateTransition(State* pNewState, STATE_TRANSITION transitionSetting)
	{
		m_pEnqueuedState = pNewState;
		m_EnqueuedTransitionAction = transitionSetting;
	}

	bool StateManager::Tick(Timestamp delta)
	{
		bool hasTransitioned = false;

		if (!m_States.empty())
		{
			m_States.top()->Tick(delta);
		}

		if (m_pEnqueuedState)
		{
			TransitionState();
			hasTransitioned = true;
		}

		return hasTransitioned;
	}

	void StateManager::FixedTick(Timestamp delta)
	{
		if (!m_States.empty())
		{
			m_States.top()->FixedTick(delta);
		}
	}

	void StateManager::TransitionState()
	{
		switch (m_EnqueuedTransitionAction)
		{
		case STATE_TRANSITION::PUSH:
			m_pECS->AddRegistryPage();
			m_States.push(m_pEnqueuedState);
			break;
		case STATE_TRANSITION::POP:
			SAFEDELETE(m_States.top());
			m_pECS->DeleteTopRegistryPage();
			m_States.pop();

			if (!m_States.empty()) {
				m_pECS->ReinstateTopRegistryPage();
				m_States.top()->Resume();
			}
			break;
		case STATE_TRANSITION::PAUSE_AND_PUSH:
			m_States.top()->Pause();
			m_pECS->DeregisterTopRegistryPage();

			m_pECS->AddRegistryPage();
			m_States.push(m_pEnqueuedState);
			break;
		case STATE_TRANSITION::POP_AND_PUSH:
			m_pECS->DeleteTopRegistryPage();
			delete m_States.top();
			m_States.pop();

			m_pECS->AddRegistryPage();
			m_States.push(m_pEnqueuedState);
			break;
		}

		m_pEnqueuedState->Init();
		m_pEnqueuedState = nullptr;
	}
}
