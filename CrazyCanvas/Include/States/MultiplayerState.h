#pragma once
#include "Game/State.h"

#include "GUI/MultiplayerGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

// TEMP - DELTE IF PULLREQUEST
#include "GUI/LobbyGUI.h"

class MultiplayerState : public LambdaEngine::State
{
public:
	MultiplayerState() = default;
	~MultiplayerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
	Noesis::Ptr<LobbyGUI> m_MultiplayerGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
