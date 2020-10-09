#pragma once
#include "Game/State.h"

#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"


class MainMenuState : public LambdaEngine::State
{
public:
	MainMenuState() = default;
	~MainMenuState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;


private:
	Noesis::Ptr<MainMenuGUI> m_MainMenuGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
