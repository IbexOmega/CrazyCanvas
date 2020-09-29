#pragma once

#include "Containers/String.h"

#include "NsGui/StackPanel.h"

class GUITest : public Noesis::StackPanel
{
public:
	GUITest();
	~GUITest();

	bool Init(const LambdaEngine::String& xamlFile);

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

private:
	Noesis::Ptr<Noesis::IView> m_View;

	NS_IMPLEMENT_INLINE_REFLECTION_(GUITest, Noesis::StackPanel)
};