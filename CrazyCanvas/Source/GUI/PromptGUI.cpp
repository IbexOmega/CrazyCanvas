#include "GUI/PromptGUI.h"

#include "NoesisPCH.h"

#include "Math/Math.h"
#include "Teams/TeamHelper.h"
#include "Log/Log.h"

PromptGUI::PromptGUI()
{
	Noesis::GUI::LoadComponent(this, "PromptGUI.xaml");

	m_pPromptStoryboard					= FindResource<Noesis::Storyboard>("PromptStoryboard");
	m_pPromptVisibilityStoryboard		= FindResource<Noesis::Storyboard>("PromptVisibilityStoryboard");
	m_pPromptTextblock					= FindName<Noesis::TextBlock>("PROMPT_TEXT");
	
	m_pSmallPromptStoryboard			= FindResource<Noesis::Storyboard>("SmallPromptStoryboard");
	m_pSmallPromptVisibilityStoryboard	= FindResource<Noesis::Storyboard>("SmallPromptVisibilityStoryboard");
	m_pSmallPromptTextblock				= FindName<Noesis::TextBlock>("SMALL_PROMPT_TEXT");
}

PromptGUI::~PromptGUI()
{
}

bool PromptGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void PromptGUI::DisplayPrompt(const LambdaEngine::String& promptMessage, uint8 teamIndex)
{
	Noesis::SolidColorBrush* pBrush = (Noesis::SolidColorBrush*)m_pPromptTextblock->GetForeground();

	if (teamIndex != UINT8_MAX)
	{
		glm::vec3 color = TeamHelper::GetTeamColor(teamIndex);
		Noesis::Color teamColor = Noesis::Color(color.r, color.g, color.b);
		pBrush->SetColor(teamColor);
	}
	else
	{
		pBrush->SetColor(Noesis::Color::Red());
	}

	m_pPromptTextblock->SetText(promptMessage.c_str());
	m_pPromptVisibilityStoryboard->Begin();
	m_pPromptStoryboard->Begin();
}

void PromptGUI::DisplaySmallPrompt(const LambdaEngine::String& promptMessage)
{
	Noesis::SolidColorBrush* pBrush = (Noesis::SolidColorBrush*)m_pSmallPromptTextblock->GetForeground();

	pBrush->SetColor(Noesis::Color::Red());

	m_pSmallPromptTextblock->SetText(promptMessage.c_str());

	m_pSmallPromptVisibilityStoryboard->Begin();
	m_pSmallPromptStoryboard->Begin();
}

void PromptGUI::CancelSmallPrompt()
{
	m_pSmallPromptVisibilityStoryboard->Stop();
	m_pSmallPromptStoryboard->Stop();
}
