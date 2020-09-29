#include "GUI/GUITest.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

GUITest::GUITest()
{
}

GUITest::~GUITest()
{
	m_View.Reset();
}

bool GUITest::Init(const LambdaEngine::String& xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	m_View = Noesis::GUI::CreateView(this);
	LambdaEngine::GUIApplication::SetView(m_View);

	return true;
}

bool GUITest::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	//NS_CONNECT_EVENT(Noesis::Button, Click, OnButton1Click);
	//NS_CONNECT_EVENT(Noesis::Button, Click, OnButton2Click);
	return false;
}

void GUITest::OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	LOG_WARNING("PP1");
}

void GUITest::OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	LOG_WARNING("PP2");
}

