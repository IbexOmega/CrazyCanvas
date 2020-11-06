#pragma once
#include "Game/State.h"

#include "Containers/TArray.h"

#include "ECS/ECSCore.h"
#include "ECS/Entity.h"

#include "GUI/GUITest.h"

#include "Rendering/IRenderGraphCreateHandler.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/MeshPaintHandler.h"

#include <NsCore/Ptr.h>
#include <NsGui/IView.h>

#include "Multiplayer/MultiplayerClient.h"

namespace LambdaEngine
{
	class RenderGraphEditor;
}

class GUITest;

class SandboxState : public LambdaEngine::State, public LambdaEngine::IRenderGraphCreateHandler
{
public:
	SandboxState();
	~SandboxState();

	void Init() override final;

	void Resume() override final;
	void Pause() override final;

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	void OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph) override final;

	void RenderImgui();

private:
	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[10];
	LambdaEngine::Entity m_Emitters[10];

	LambdaEngine::RenderGraphEditor*	m_pRenderGraphEditor	= nullptr;
	bool m_ECSVisualization				= false;
	bool m_RenderGraphWindow			= false;
	bool m_ShowDemoWindow				= false;
	bool m_DebuggingWindow				= false;
	bool m_ShowTextureDebuggingWindow	= false;
	bool m_DirLightDebug				= false;
	bool m_DebugEmitters				= false;
	
	LambdaEngine::TArray<LambdaEngine::ImGuiTexture> m_TextureDebuggingNames;
	LambdaEngine::TArray<LambdaEngine::Entity> m_Entities;

	/* Systems */
	MultiplayerClient m_MultiplayerClient;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	MeshPaintHandler m_MeshPaintHandler;
};
