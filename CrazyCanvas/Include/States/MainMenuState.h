#pragma once
#include "Game/State.h"

#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"


namespace LambdaEngine
{
	class IMusic;
}

class MainMenuState : public LambdaEngine::State
{
public:
	MainMenuState() = default;
	~MainMenuState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
	Noesis::Ptr<MainMenuGUI> m_MainMenuGUI;
	Noesis::Ptr<Noesis::IView> m_View;

	GUID_Lambda m_MainMenuMusicGUID = GUID_NONE;
};
