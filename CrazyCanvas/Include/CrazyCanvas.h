#pragma once
#include "Game/Game.h"

#include "Application/API/ApplicationEventHandler.h"
#include "Math/Math.h"

#include <argh/argh.h>

class CrazyCanvas : public LambdaEngine::Game, public LambdaEngine::ApplicationEventHandler
{
public:
	CrazyCanvas(const argh::parser& flagParser);
	~CrazyCanvas() = default;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta) override;
	virtual void FixedTick(LambdaEngine::Timestamp delta) override;

	void Render(LambdaEngine::Timestamp delta);

private:
	bool LoadRendererResources();

	static void PrintBenchmarkResults();
};
