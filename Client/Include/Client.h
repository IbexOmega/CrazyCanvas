#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

class Client :
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler
{
public:
	Client();
	~Client();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	
};
