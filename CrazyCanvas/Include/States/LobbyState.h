#pragma once
#include "Game/State.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Lobby/LobbyClient.h"

class LobbyState : public LambdaEngine::State
{
public:
	LobbyState() = default;
	~LobbyState();

protected:
	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
	LobbyClient m_LobbyClient;

private:
	/*Noesis::Ptr<LobbyGUI> m_LobbyGUI;
	Noesis::Ptr<Noesis::IView> m_View;*/
};
