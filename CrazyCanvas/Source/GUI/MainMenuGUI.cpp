#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
}

MainMenuGUI::~MainMenuGUI()
{
}
