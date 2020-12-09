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
	Noesis::BitmapImage* pBitmap = new Noesis::BitmapImage();
	if (isFriendly)
		pBitmap->SetUriSource(Noesis::Uri("HitFriendlyIndicator.png"));
	else
		pBitmap->SetUriSource(Noesis::Uri("HitIndicator.png"));

	m_pDamageIndicator->SetSource(pBitmap);

	m_pDamageIndicatorStoryboard->Begin();
}