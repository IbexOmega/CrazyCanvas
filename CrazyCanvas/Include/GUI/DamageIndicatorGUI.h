#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/Image.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

class DamageIndicatorGUI : public Noesis::UserControl
{
public:
	DamageIndicatorGUI();
	~DamageIndicatorGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void DisplayIndicator();

private:
	Noesis::Storyboard* m_pDamageIndicatorStoryboard = nullptr;
	Noesis::Image* m_pIndicatorImage = nullptr;

	NS_IMPLEMENT_INLINE_REFLECTION_(DamageIndicatorGUI, Noesis::UserControl, "CrazyCanvas.DamageIndicatorGUI")

};