#pragma once

#include "Containers/TArray.h"
#include "Time/API/Timestamp.h"

#include <stack>
#include <vector>
#include <queue>

namespace LambdaEngine
{
    class ECSCore;
    class State;

    // What to do when pushing a new state
    enum class STATE_TRANSITION {
        PUSH,
        POP,
        PAUSE_AND_PUSH,
        POP_AND_PUSH
    };

    class LAMBDA_API StateManager
    {
    public:
        StateManager();
        ~StateManager();

        bool Init(ECSCore* pECS);
        bool Release();

        // The enqueued transition happens at the end of StateManager::Tick
        void EnqueueStateTransition(State* pNewState, STATE_TRANSITION transitionSetting);
        void EnqueueStatePop() { EnqueueStateTransition(nullptr, STATE_TRANSITION::POP); }

        void Tick(Timestamp delta);
        void FixedTick(Timestamp delta);

        static StateManager* GetInstance() { return &s_Instance; }

    private:
        void TransitionState();

    private:
        static StateManager s_Instance;

        // Stack of game states with vector as storage class to allow for contiguous element storage
        std::stack<State*, std::vector<State*>> m_States;
        // Old states can't be deleted during transitions because their resources might be needed by the new states. They are instead enqueued for deletion.
        std::queue<State*> m_StatesToDelete;
        ECSCore* m_pECS;

        State* m_pEnqueuedState;
        STATE_TRANSITION m_EnqueuedTransitionAction;
    };
}
