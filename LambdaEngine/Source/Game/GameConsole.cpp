#include "GameConsole.h"
#include <imgui.h>

void LambdaEngine::GameConsole::Render()
{
	if (ImGui::Begin("Console"))
	{
		static char buf[32];
		ImGui::InputText("", buf, 256);
	}
}
