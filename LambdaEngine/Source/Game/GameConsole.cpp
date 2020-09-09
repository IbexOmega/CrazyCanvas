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

void LambdaEngine::GameConsole::BindCommand(ConsoleCommand cmd, std::function<void(TArray<Arg>& arguments, std::unordered_map<std::string, Flag>& flags)> callback)
{
	m_CommandMap[cmd.GetName()] = std::pair<ConsoleCommand, std::function<void(TArray<Arg>& arguments, std::unordered_map<std::string, Flag>& flags)>>(cmd, callback);
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
	command.erase(0, pos + std::string(" ").length());
	
	auto it = m_CommandMap.find(token);

	if (it == m_CommandMap.end())
	{
		PushError("Command '" + token + "' not found.");
		return 0;
	}

	ConsoleCommand& cmd = it->second.first;

	Flag* preFlag = nullptr;
	std::unordered_map<std::string, Flag> flags;
	
	bool wasFlag = false;
	uint32 index = 0;
	while (((pos = command.find(" ")) != std::string::npos) || ((pos = command.length()) > 0))
	{
		Arg arg;
		token = command.substr(0, pos);
		if (std::regex_match(token, std::regex("-[[:alpha:]].*")))
		{
			wasFlag = true;
			Flag flag;
			flag.name = token.substr(1);

			if (cmd.GetFlags().find(flag.name) == cmd.GetFlags().end())
			{
				PushError("'" + token + "' is an invalid flag!");
				return 0;
			}

			flags[flag.name] = flag;
			preFlag = &flags[flag.name];
		}
		else
		{
			FillArg(arg, token);
			if (wasFlag)
			{
				if (cmd.GetFlags()[preFlag->name].arg.type != Arg::EMPTY)
				{
					preFlag->arg = arg;
				}
				else
				{
					bool res = AddArg(index, arg, cmd);
					if (!res) return 0;
					index++;
				}
				wasFlag = false;
			}
			else
			{
				bool res = AddArg(index, arg, cmd);
				if (!res) return 0;
				index++;
			}
		}
		
		if (std::string::npos != command.find(" "))
			command.erase(0, pos + std::string(" ").length());
		else
			command.erase(0, pos);
	}

	uint32 size = cmd.GetArguments().GetSize();
	if (index != size) // Error too few arguments!
	{
		PushError("Too few arguments!");
		return 0;
	}

	item.str = data;
	item.color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	m_Items.PushBack(item);

	// Call function
	it->second.second(cmd.GetArguments(), flags);

	return 0;
}

void LambdaEngine::GameConsole::FillArg(Arg& arg, std::string token)
{
	arg.type = Arg::STRING;
	strcpy(arg.value.str, token.c_str());

	if (std::regex_match(token, std::regex("-[0-9]+")))
	{
		arg.type = Arg::INT;
		arg.value.i = std::stoi(token);
	}
	else if (std::regex_match(token, std::regex("[0-9]+")))
	{
		arg.type = Arg::INT;
		arg.value.i = std::stoi(token);
	}
	else if (std::regex_match(token, std::regex("(-[0-9]*\.[0-9]+)|(-[0-9]+\.[0-9]*)")))
	{
		arg.type = Arg::FLOAT;
		arg.value.f = std::stof(token);
	}
	else if (std::regex_match(token, std::regex("([0-9]*\.[0-9]+)|([0-9]+\.[0-9]*)")))
	{
		arg.type = Arg::FLOAT;
		arg.value.f = std::stof(token);
	}
	std::for_each(token.begin(), token.end(), [](char& c) { c = std::tolower(c); });
	if (std::regex_match(token, std::regex("(false)|(true)")))
	{
		arg.type = Arg::BOOL;
		arg.value.b = token == "false" ? false : true;
	}
}

bool LambdaEngine::GameConsole::AddArg(uint32 index, Arg arg, ConsoleCommand& cmd)
{
	uint32 size = cmd.GetArguments().GetSize();
	if (index >= size) // Error too many arguments
	{
		PushError("Too many arguments!");
		return false;
	}
	if (cmd.GetArguments()[index].type != arg.type) // Error wrong type
	{
		PushError("Wrong argument type!");
		return false;
	}
	cmd.GetArguments()[index].value = arg.value;
	return true;
}

void LambdaEngine::GameConsole::PushError(const std::string& msg)
{
	Item item = {};
	item.str = "Error:" + msg;
	item.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
	m_Items.PushBack(item);
}
