#pragma once
#pragma once

#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"

class MainMenuGUI : public Noesis::Grid
{
public:
	MainMenuGUI(const LambdaEngine::String& xamlFile);
	~MainMenuGUI();


private:
	NS_IMPLEMENT_INLINE_REFLECTION_(MainMenuGUI, Noesis::Grid)
};