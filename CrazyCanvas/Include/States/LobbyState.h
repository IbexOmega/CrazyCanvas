#pragma once
#include "Game/State.h"

#include "GUI/LobbyGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"


class LobbyState : public LambdaEngine::State
{
public:
	LobbyState() = default;
	~LobbyState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
	Noesis::Ptr<LobbyGUI> m_LobbyGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
