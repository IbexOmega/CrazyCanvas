#pragma once
#include "Game/State.h"

#include "GUI/HUDGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"


class HUDState
{
public:
	HUDState() = default;
	~HUDState();

	void Init();

	/*void Resume() override final {};
	void Pause() override final {};
	*/
	void Tick(LambdaEngine::Timestamp delta);


private:
	Noesis::Ptr<HUDGUI> m_HUDGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
