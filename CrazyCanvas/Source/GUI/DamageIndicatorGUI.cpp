#include "GUI/DamageIndicatorGUI.h"

#include "NoesisPCH.h"


DamageIndicatorGUI::DamageIndicatorGUI()
{
	Noesis::GUI::LoadComponent(this, "DamageIndicatorGUI.xaml");

	m_pDamageIndicatorStoryboard	= FindResource<Noesis::Storyboard>("DamageIndicatorStoryboard");
	m_pDamageIndicator				= FindName<Noesis::Image>("DamageIndicator");
}

DamageIndicatorGUI::~DamageIndicatorGUI()
{
}

bool DamageIndicatorGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void DamageIndicatorGUI::DisplayIndicator(bool isFriendly)
{
	UNREFERENCED_VARIABLE(isFriendly);
	Noesis::Ptr<Noesis::BitmapImage> bitmap = *new Noesis::BitmapImage();
	if (isFriendly)
		bitmap->SetUriSource(Noesis::Uri("HitFriendlyIndicator.png"));
	else
		bitmap->SetUriSource(Noesis::Uri("HitIndicator.png"));

	m_pDamageIndicator->SetSource(bitmap);

	m_pDamageIndicatorStoryboard->Begin();
}