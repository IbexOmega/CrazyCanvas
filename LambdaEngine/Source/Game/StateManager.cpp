#include "Game/StateManager.h"

#include "ECS/ECSCore.h"
#include "Game/State.h"

namespace LambdaEngine
{
    StateManager::StateManager(ECSCore* pECS)
        :m_pECS(pECS),
        m_pEnqueuedState(nullptr),
        m_EnqueuedTransitionAction(STATE_TRANSITION::PUSH)
    {}

    StateManager::~StateManager()
    {
        while (!m_StatesToDelete.empty())
        {
            delete m_StatesToDelete.front();
            m_StatesToDelete.pop();
        }

        while (!m_States.empty())
        {
            delete m_States.top();
            m_States.pop();
        }
    }

    void StateManager::EnqueueStateTransition(State* pNewState, STATE_TRANSITION transitionSetting)
    {
        m_pEnqueuedState = pNewState;
        m_EnqueuedTransitionAction = transitionSetting;
    }

    void StateManager::Tick(float dt)
    {
        if (!m_States.empty())
        {
            m_States.top()->Tick(dt);
        }

        if (m_pEnqueuedState)
        {
            TransitionState();
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
                delete m_States.top();
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
                m_StatesToDelete.push(m_States.top());
                m_States.pop();

                m_pECS->AddRegistryPage();
                m_States.push(m_pEnqueuedState);
                break;
        }

        m_pEnqueuedState->Init();
        m_pEnqueuedState = nullptr;
    }
}
