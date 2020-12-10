#include "GUI/CountdownGUI.h"

#include "NoesisPCH.h"

#include "Teams/TeamHelper.h"
#include "Lobby/PlayerManagerClient.h"
#include "Math/Math.h"

CountdownGUI::CountdownGUI()
{
	Noesis::GUI::LoadComponent(this, "CountdownGUI.xaml");

	m_pCountdownStoryboard = FindResource<Noesis::Storyboard>("CountdownStoryboard");
	m_pCountdownTextblock = FindName<Noesis::TextBlock>("textBlock");

	Noesis::Ptr<Noesis::SolidColorBrush> brush = *new Noesis::SolidColorBrush();

	const glm::vec3& promptColor = TeamHelper::GetTeamColor(PlayerManagerClient::GetPlayerLocal()->GetTeam());
	Noesis::Color color(promptColor.r, promptColor.g, promptColor.b);

	brush->SetColor(color);

	m_pCountdownTextblock->SetForeground(brush);
}

CountdownGUI::~CountdownGUI()
{
}

bool CountdownGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void CountdownGUI::UpdateCountdown(uint8 countDownTime)
{
	if (countDownTime == 0)
	{
		m_pCountdownTextblock->SetText("GO");
		SetOpacity(1.0f);
	}
	else if (countDownTime == UINT8_MAX)
	{
		SetOpacity(0.0f);
	}
	else
	{
		m_pCountdownTextblock->SetText(std::to_string(countDownTime).c_str());
		SetOpacity(1.0f);
	}

	m_pCountdownStoryboard->Begin();
}