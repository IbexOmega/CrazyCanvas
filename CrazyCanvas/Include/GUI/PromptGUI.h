#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/Textblock.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

class PromptGUI : public Noesis::UserControl
{
public:
	PromptGUI();
	~PromptGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void DisplayPrompt(const LambdaEngine::String& promptMessage);

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(PromptGUI, Noesis::UserControl, "CrazyCanvas.PromptGUI")

private:
	Noesis::Storyboard* m_pPromptStoryboard				= nullptr;
	Noesis::Storyboard* m_pPromptVisibilityStoryboard	= nullptr;
	Noesis::TextBlock* m_pPromptTextblock				= nullptr;

};