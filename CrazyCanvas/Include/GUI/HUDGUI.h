#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/ObservableCollection.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

class HUDGUI : public Noesis::Grid
{

public:
	HUDGUI(const LambdaEngine::String& xamlFile);
	~HUDGUI();

	void OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;

	bool ApplyDamage(float damage);

private:

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	float m_LifeMaxHeight;
	float m_Damage;
	float m_Health;
};