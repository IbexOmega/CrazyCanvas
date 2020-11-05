#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/Textblock.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

class CountdownGUI : public Noesis::UserControl
{
public:
	CountdownGUI();
	~CountdownGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void UpdateCountdown(uint8 countDownTime);

private:
	Noesis::Storyboard* m_pCountdownStoryboard = nullptr;
	Noesis::TextBlock*	m_pCountdownTextblock = nullptr;

	NS_IMPLEMENT_INLINE_REFLECTION_(CountdownGUI, Noesis::UserControl, "CrazyCanvas.CountdownGUI")

};