#pragma once

#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"

class GUITest : public Noesis::Grid
{
public:
	GUITest(const LambdaEngine::String& xamlFile);
	~GUITest();

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(GUITest, Noesis::Grid)
};