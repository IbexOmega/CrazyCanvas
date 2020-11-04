#pragma once
#include "Game/State.h"

#include "ECS/System.h"

#include "GUI/HUDGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Events/MatchEvents.h"

class HUDSystem : public LambdaEngine::System
{
public:
	HUDSystem() = default;
	~HUDSystem();

	void Init();

	/*void Resume() override final {};
	void Pause() override final {};
	*/
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;
	void FixedTick(LambdaEngine::Timestamp delta);

private:
	bool OnMatchCountdownEvent(const MatchCountdownEvent& event);

private:
	LambdaEngine::IDVector m_WeaponEntities;

	Noesis::Ptr<HUDGUI> m_HUDGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
