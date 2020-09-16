#pragma once
#include "Game/Game.h"

#include "Application/API/ApplicationEventHandler.h"
#include "CameraTrack.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct GameObject;

	class RenderGraph;
	class Renderer;
	class ResourceManager;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class Scene;
	class Sampler;

	class RenderGraphEditor;
}

class CrazyCanvas : public LambdaEngine::Game, public LambdaEngine::ApplicationEventHandler
{
public:
	CrazyCanvas();
	~CrazyCanvas();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta) override;
	virtual void FixedTick(LambdaEngine::Timestamp delta) override;

	void Render(LambdaEngine::Timestamp delta);

private:
	bool LoadRendererResources();

	static void PrintBenchmarkResults();

private:
	CameraTrack m_CameraTrack;

	LambdaEngine::Scene* m_pScene = nullptr;
};
