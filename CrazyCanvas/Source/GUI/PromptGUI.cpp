#include "GUI/PromptGUI.h"

#include "NoesisPCH.h"

PromptGUI::PromptGUI()
{
	Noesis::GUI::LoadComponent(this, "PromptGUI.xaml");

	m_pPromptStoryboard				= FindResource<Noesis::Storyboard>("PromptStoryboard");
	m_pPromptVisibilityStoryboard	= FindResource<Noesis::Storyboard>("PromptVisibilityStoryboard");
	m_pPromptTextblock				= FindName<Noesis::TextBlock>("PROMPT_TEXT");
}

PromptGUI::~PromptGUI()
{
}

bool PromptGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void PromptGUI::DisplayPrompt(const LambdaEngine::String& promptMessage)
{
	m_pPromptTextblock->SetText(promptMessage.c_str());
	m_pPromptVisibilityStoryboard->Begin();
	m_pPromptStoryboard->Begin();
}