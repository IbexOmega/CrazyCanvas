#pragma once
#include "Game/Game.h"

#include "Application/API/ApplicationEventHandler.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

#include <argh/argh.h>

#include "MeshPaint/MeshPaintHandler.h"

class CrazyCanvas : public LambdaEngine::Game, public LambdaEngine::ApplicationEventHandler
{
public:
	CrazyCanvas(const argh::parser& flagParser);
	~CrazyCanvas();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta) override;
	virtual void FixedTick(LambdaEngine::Timestamp delta) override;

	void Render(LambdaEngine::Timestamp delta);

private:
	bool RegisterGUIComponents();
	bool InitRendererResources();
	bool BindComponentTypeMasks();

private:
	MeshPaintHandler m_MeshPaintHandler;
};
