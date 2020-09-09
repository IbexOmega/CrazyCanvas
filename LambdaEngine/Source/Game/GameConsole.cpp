#include "Game/GameConsole.h"
#include "Input/API/Input.h"
#include <regex>
#include <imgui.h>

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
	// Toggle console when pressing § (Button beneath Escape)
	static bool s_Active = false;
	static bool s_Toggle = false;

	if (Input::IsKeyDown(EKey::KEY_GRAVE_ACCENT) & !s_Active)
	{
		s_Active = true;
		s_Toggle ^= 1;
		LOG_INFO("TILDE!");
	}
	else if (Input::IsKeyUp(EKey::KEY_GRAVE_ACCENT))
		s_Active = false;

	// Do not draw if not active.
	if (!s_Toggle)
		return;

	// Draw a console window at the top right of the viewport.
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Console", (bool*)0, ImGuiWindowFlags_NoMove))
	{
		bool hasFocus = false;

		// History
		const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		for (int i = 0; i < m_Items.GetSize(); i++)
		{
			Item& item = m_Items[i];
			const char* str = item.str.c_str();
			ImVec4 color = ImVec4(item.color.r, item.color.g, item.color.b, item.color.a);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(str);
			ImGui::PopStyleColor();
		}

		if (m_ScrollToBottom | (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		m_ScrollToBottom = false;

		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command line
		static char s_Buf[256];
		ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("Input", s_Buf, 256, input_text_flags))
		{
			if (s_Buf[0])
				ExecCommand(std::string(s_Buf));
			strcpy(s_Buf, "");
			hasFocus = true;
		}

		if (s_Active || hasFocus)
		{
			ImGui::SetItemDefaultFocus();
			ImGui::SetKeyboardFocusHere(-1); // Set focus to the text field.
		}

	}
	ImGui::End();
}

void LambdaEngine::GameConsole::BindCommand(ConsoleCommand cmd, std::function<void(TArray<Arg>& arguments)> callback)
{
	m_CommandMap[cmd.GetName()] = std::pair<ConsoleCommand, std::function<void(TArray<Arg>& arguments)>>(cmd, callback);
}

LambdaEngine::GameConsole& LambdaEngine::GameConsole::Get()
{
	static GameConsole console;
	return console;
}

LambdaEngine::GameConsole::GameConsole()
{
}

LambdaEngine::GameConsole::~GameConsole()
{
}

int LambdaEngine::GameConsole::ExecCommand(std::string& data)
{
	size_t pos = 0;
	std::string token;
	std::string command = data;
	Item item = {};
	m_ScrollToBottom = true;

	pos = command.find(" ");
	token = command.substr(0, pos);
	LOG_INFO(token.c_str());
	command.erase(0, pos + std::string(" ").length());
	
	auto it = m_CommandMap.find(token);

	if (it == m_CommandMap.end())
	{
		item.str = "Error: Command '" + token + "' not found.";
		item.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
		m_Items.PushBack(item);
		return 0;
	}

	while (((pos = command.find(" ")) != std::string::npos) || ((pos = command.length()) > 0))
	{
		LOG_INFO("pos: %d", pos);
		LOG_INFO("npos: %d", command.find(" ") == std::string::npos);
		token = command.substr(0, pos);
		if (std::regex_match(token, std::regex("-[[:w:]].*")))
			LOG_INFO("FLAG FOUND");
		if (std::regex_match(token, std::regex("-[0-9]+")))
			LOG_INFO("NUMBER FOUND");
		
		LOG_INFO(token.c_str());
		if (std::string::npos != command.find(" "))
			command.erase(0, pos + std::string(" ").length());
		else
			command.erase(0, pos);
	}

	item.str = data;
	item.color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	m_Items.PushBack(item);

	return 0;
}
