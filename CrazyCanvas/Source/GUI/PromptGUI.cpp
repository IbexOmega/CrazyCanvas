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

void PromptGUI::DisplayPrompt(const LambdaEngine::String& promptMessage, const uint8 teamIndex)
{
	Noesis::SolidColorBrush* pBrush = new Noesis::SolidColorBrush();

	UNREFERENCED_VARIABLE(teamIndex);

	/*if (teamIndex != UINT8_MAX)
	{
		glm::vec3 promptColor = TeamHelper::GetTeamColor(teamIndex);
		Noesis::Color color(promptColor.r, promptColor.g, promptColor.b);

		pBrush->SetColor(color);
	}
	else
		pBrush->SetColor(Noesis::Color::Red());*/

	pBrush->SetColor(Noesis::Color::White());

	m_pPromptTextblock->SetForeground(pBrush);

	m_pPromptTextblock->SetText(promptMessage.c_str());
	m_pPromptVisibilityStoryboard->Begin();
	m_pPromptStoryboard->Begin();
}

void PromptGUI::DisplaySmallPrompt(const LambdaEngine::String& promptMessage)
{
	Noesis::SolidColorBrush* pBrush = new Noesis::SolidColorBrush();

	pBrush->SetColor(Noesis::Color::Red());

	m_pSmallPromptTextblock->SetForeground(pBrush);
	m_pSmallPromptTextblock->SetText(promptMessage.c_str());

	m_pSmallPromptVisibilityStoryboard->Begin();
	m_pSmallPromptStoryboard->Begin();
}

void PromptGUI::CancelSmallPrompt()
{
	m_pSmallPromptVisibilityStoryboard->Stop();
	m_pSmallPromptStoryboard->Stop();
}
