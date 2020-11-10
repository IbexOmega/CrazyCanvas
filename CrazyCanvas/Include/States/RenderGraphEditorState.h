#pragma once
#include "Game/State.h"

#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"


class RenderGraphEditorState : public LambdaEngine::State
{
public:
	RenderGraphEditorState() = default;
	~RenderGraphEditorState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override;

	void RenderImgui();

private:
	Noesis::Ptr<MainMenuGUI> m_MainMenuGUI;
	Noesis::Ptr<Noesis::IView> m_View;

	LambdaEngine::RenderGraphEditor* m_pRenderGraphEditor = nullptr;
};