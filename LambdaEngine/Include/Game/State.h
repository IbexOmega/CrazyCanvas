#pragma once

#include "Game/StateManager.h"

namespace LambdaEngine
{
    class ECSCore;

    class State
    {
    public:
        State(StateManager* pStateManager, ECSCore* pECS);
        State(State* pOther);
        virtual ~State() = 0 {};

        virtual void Init() = 0;

        virtual void Resume() = 0;
        virtual void Pause() = 0;

        virtual void Tick(float dt) = 0;

    protected:
        ECSCore* m_pECS;
        StateManager* m_pStateManager;
    };
}
