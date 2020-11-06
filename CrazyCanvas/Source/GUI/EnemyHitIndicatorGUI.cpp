#include "GUI/EnemyHitIndicatorGUI.h"

#include "NoesisPCH.h"


EnemyHitIndicatorGUI::EnemyHitIndicatorGUI()
{
	Noesis::GUI::LoadComponent(this, "EnemyHitIndicatorGUI.xaml");

	m_pEnemyHitIndicatorStoryboard = FindResource<Noesis::Storyboard>("EnemyHitIndicatorStoryboard");
	m_pIndicatorImage = FindName<Noesis::Image>("HitIndicator");
}

EnemyHitIndicatorGUI::~EnemyHitIndicatorGUI()
{
}

bool EnemyHitIndicatorGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void EnemyHitIndicatorGUI::DisplayIndicator()
{
	m_pEnemyHitIndicatorStoryboard->Begin();
}