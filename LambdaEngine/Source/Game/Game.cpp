#include "Game/Game.h"

namespace LambdaEngine
{
    Game* Game::s_pInstance = nullptr;

    Game::Game()
    {
        ASSERT(s_pInstance == nullptr);
        s_pInstance = this;
    }

    Game::~Game()
    {
        ASSERT(s_pInstance != nullptr);
        s_pInstance = nullptr;
    }
}
