#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/Image.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

class EnemyHitIndicatorGUI : public Noesis::UserControl
{
public:
	EnemyHitIndicatorGUI();
	~EnemyHitIndicatorGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void DisplayIndicator();

private:
	Noesis::Storyboard* m_pEnemyHitIndicatorStoryboard = nullptr;

	NS_IMPLEMENT_INLINE_REFLECTION_(EnemyHitIndicatorGUI, Noesis::UserControl, "CrazyCanvas.EnemyHitIndicatorGUI")

};
