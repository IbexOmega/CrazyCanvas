#pragma once

#include "Game/State.h"
#include "ECS/Entity.h"

#include "ECS/ECSCore.h"
#include "Containers/TArray.h"

#include "Application/API/Events/KeyEvents.h"

#include "NsCore/Ptr.h"
#include "NsGui/IView.h"

namespace LambdaEngine
{
	class StateManager;
	class ECSCore;
}

class GUITest;

class SandboxState : public LambdaEngine::State
{
public:
	SandboxState();
	SandboxState(LambdaEngine::State* pOther);
	~SandboxState();

	void Init() override final;

	void Resume() override final;
	void Pause() override final;

	void Tick(LambdaEngine::Timestamp delta);

private:
	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[3];

	Noesis::Ptr<GUITest> m_GUITest;
	Noesis::Ptr<Noesis::IView> m_View;
};