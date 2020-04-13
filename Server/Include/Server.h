#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include <set>

namespace LambdaEngine
{
	class IClientTCPHandler;
	class IClientUDP;
}

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler
{
public:
	Server();
	~Server();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	void UpdateTitle();

private:
	
};
