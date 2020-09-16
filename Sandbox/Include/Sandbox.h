#pragma once
#include "Game/Game.h"

#include "Application/API/Events/KeyEvents.h"

#include "Rendering/IRenderGraphCreateHandler.h"

#include "Containers/String.h"
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
	class Buffer;

	class RenderGraphEditor;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::IRenderGraphCreateHandler
{
	static constexpr const uint32 NUM_POINT_LIGHTS = 2;

	struct PointLight //Testing
	{
		glm::vec4 Position;
		glm::mat4 Transforms[6];
	};

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

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta) override;
	virtual void FixedTick(LambdaEngine::Timestamp delta) override;

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

	LambdaEngine::Buffer*					m_pPointLightsBuffer;

	bool									m_RenderGraphWindow;
	bool									m_ShowDemoWindow;
	bool									m_DebuggingWindow;

	bool					m_ShowTextureDebuggingWindow	= false;
	LambdaEngine::String	m_TextureDebuggingName			= "";
	GUID_Lambda				m_TextureDebuggingShaderGUID	= GUID_NONE;

};
