#pragma once

#include "Game/Game.h"

#include "Input/IKeyboardHandler.h"
#include "Input/IMouseHandler.h"

class Sandbox : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IMouseHandler
{
public:
	Sandbox();
	~Sandbox();

	// Inherited via Game
	virtual void Tick() override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key) override;
	virtual void OnKeyUp(LambdaEngine::EKey key) override;

	// Inherited via IMouseHandler
	virtual void OnMouseMove(int32 x, int32 y) override;
	virtual void OnButtonPressed(LambdaEngine::EMouseButton button) override;
	virtual void OnButtonReleased(LambdaEngine::EMouseButton button) override;
	virtual void OnScroll(int32 delta) override;
};