#include "Game/GameConsole.h"

#include <imgui.h>

LambdaEngine::TArray<char*> LambdaEngine::GameConsole::m_Items;

bool LambdaEngine::GameConsole::Init()
{
	return true;
}

bool LambdaEngine::GameConsole::Release()
{
	return true;
}

void LambdaEngine::GameConsole::Render()
{
	if (ImGui::Begin("Console"))
	{
		static char s_Buf[256];
		bool reclaimFocus = false;
		ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::EndChild();

		ImGui::Separator();

		if (ImGui::InputText("Input", s_Buf, 256, input_text_flags))
		{
			m_Items.PushBack(s_Buf);
			ParseText(s_Buf);
			char* s = s_Buf;
			strcpy(s, "");
			reclaimFocus = true;
		}

		ImGui::SetItemDefaultFocus();
		if (reclaimFocus)
			ImGui::SetKeyboardFocusHere(-1);


		ImGui::End();
	}
}

int LambdaEngine::GameConsole::ParseText(const char* data)
{
	LOG_INFO("Hej");

	return 0;
}