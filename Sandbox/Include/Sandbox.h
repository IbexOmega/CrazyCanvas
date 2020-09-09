#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"
#include "Rendering/IRenderGraphCreateHandler.h"

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
	class Camera;
	class Sampler;

	class RenderGraphEditor;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::EventHandler, public LambdaEngine::IRenderGraphCreateHandler
{
	struct InstanceIndexAndTransform
	{
		uint32		InstanceIndex;
		glm::vec3	Position;
		glm::vec4	Rotation;
		glm::vec3	Scale;
	};

public:
	Sandbox();
	~Sandbox();

	virtual void OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)	override;
	
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	void Render(LambdaEngine::Timestamp delta);

	virtual void OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph) override;

private:
	bool LoadRendererResources();

private:
	LambdaEngine::Scene*					m_pScene				= nullptr;
	LambdaEngine::Camera*					m_pCamera				= nullptr;

	LambdaEngine::RenderGraphEditor*		m_pRenderGraphEditor	= nullptr;

	LambdaEngine::TArray<InstanceIndexAndTransform>		m_InstanceIndicesAndTransforms;
	LambdaEngine::TArray<InstanceIndexAndTransform>		m_LightInstanceIndicesAndTransforms;

	float									m_DirectionalLightAngle;
	float									m_DirectionalLightStrength[4];

};
