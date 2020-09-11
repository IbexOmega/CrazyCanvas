#include "Game/GameConsole.h"
#include "Input/API/Input.h"
#include "Rendering/ImGuiRenderer.h"
#include <regex>
#include <imgui.h>

namespace LambdaEngine
{
	bool GameConsole::Init()
	{
		#ifdef LAMBDA_DEBUG
			ConsoleCommand cmdHelp;
			cmdHelp.Init("help", false);
			cmdHelp.AddFlag("d", Arg::EType::EMPTY);
			cmdHelp.AddDescription("Shows all commands descriptions.", { {"d", "Used to show debug commands also."} });
			BindCommand(cmdHelp, [this](CallbackInput& input)->void {
				for (auto i : m_CommandMap)
				{
					ConsoleCommand& cCmd = i.second.first;
					if (!cCmd.IsDebug() || (input.Flags.find("d") != input.Flags.end()))
					{
						std::string type = cCmd.IsDebug() ? " [DEBUG]" : "";
						ConsoleCommand::Description desc = cCmd.GetDescription();
						PushInfo(i.first + type + ":");
						PushMsg("\t" + desc.MainDesc, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
						for (auto flagDesc : desc.FlagDescs)
							PushMsg("\t\t-" + flagDesc.first + ": " + flagDesc.second, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
					}
				}
			});
		#else
			ConsoleCommand cmdHelp;
			cmdHelp.Init("help", false);
			cmdHelp.AddDescription("Shows all commands descriptions.");
			BindCommand(cmdHelp, [this](CallbackInput& input)->void {
				for (auto i : m_CommandMap)
				{
					ConsoleCommand& cCmd = i.second.first;
					if (!cCmd.IsDebug())
					{
						ConsoleCommand::Description desc = cCmd.GetDescription();
						PushInfo(i.first + ":");
						PushMsg("\t" + desc.MainDesc, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
						for (auto flagDesc : desc.FlagDescs)
							PushMsg("\t\t-" + flagDesc.first + ": " + flagDesc.second, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
					}
				}
			});
		#endif
		
		ConsoleCommand cmdClear;
		cmdClear.Init("clear", false);
		cmdClear.AddFlag("h", Arg::EType::EMPTY);
		cmdClear.AddDescription("Clears the visible text in the console.", { {"h", "Clears the history."} });
		BindCommand(cmdClear, [this](CallbackInput&)->void {
			m_Items.Clear();
		});

		// Test Command
		ConsoleCommand cmd;
		cmd.Init("clo", true);
		cmd.AddArg(Arg::EType::STRING);
		cmd.AddFlag("l", Arg::EType::INT);
		cmd.AddFlag("i", Arg::EType::EMPTY);
		cmd.AddDescription("Does blah and do bar.");
		GameConsole::Get().BindCommand(cmd, [](GameConsole::CallbackInput& input)->void {
			std::string s1 = input.Arguments.GetFront().Value.Str;
			std::string s2 = input.Flags.find("i") == input.Flags.end() ? "no set" : "set";
			std::string s3 = "no set";
			auto it = input.Flags.find("l");
			if (it != input.Flags.end())
				s3 = "set with a value of " + std::to_string(it->second.Arg.Value.I);
			LOG_INFO("Command Called with argument '%s' and flag i was %s and flag l was %s.", s1.c_str(), s2.c_str(), s3.c_str());
			});

		return true;
	}

	bool GameConsole::Release()
	{
		return true;
	}

	void GameConsole::Tick()
	{
		// Toggle console when pressing § (Button beneath Escape)
		static bool s_Active = false;
		static bool s_Toggle = false;

		if (Input::IsKeyDown(EKey::KEY_GRAVE_ACCENT) & !s_Active)
		{
			s_Active = true;
			s_Toggle ^= 1;
		}
		else if (Input::IsKeyUp(EKey::KEY_GRAVE_ACCENT))
			s_Active = false;

		// Do not draw if not active.
		if (!s_Toggle)
			return;

		ImVec2 popupPos;
		ImVec2 popupSize;

		ImGuiRenderer::Get().DrawUI([&]()
			{
				// Draw a console window at the top right of the viewport.
				ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Console", (bool*)0, ImGuiWindowFlags_NoMove))
				{
					bool hasFocus = false;

					// History
					const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
					ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

					// Only display visible text to see history.
					ImGuiListClipper clipper(m_Items.GetSize());
					while (clipper.Step())
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
						{
							Item& item = m_Items[i];
							const char* str = item.Str.c_str();
							ImVec4 color = ImVec4(item.Color.r, item.Color.g, item.Color.b, item.Color.a);
							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::TextUnformatted(str);
							ImGui::PopStyleColor();
						}

					if (m_ScrollToBottom | (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
						ImGui::SetScrollHereY(0.0f);
					m_ScrollToBottom = false;

					ImGui::PopStyleVar();
					ImGui::EndChild();
					ImGui::Separator();

					// Command line
					static char s_Buf[256];
					ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
					if (ImGui::InputText("Input", s_Buf, 256, input_text_flags, [](ImGuiInputTextCallbackData* data)->int {
						GameConsole* console = (GameConsole*)data->UserData;
						return console->TextEditCallback(data);
						}, (void*)this))
					{
						if (m_ActivePopupIndex != -1)
						{
							strcpy(s_Buf, m_PopupSelectedText.c_str());
							m_ActivePopupIndex = -1;
							m_Candidates.Clear();
							m_PopupSelectedText = "";
						}
						else
						{
							if (s_Buf[0])
							{
								std::string buff = std::string(s_Buf);
								ExecCommand(buff);
							}

							strcpy(s_Buf, "");
						}
						hasFocus = true;
					}

					if (s_Active || hasFocus)
					{
						ImGui::SetItemDefaultFocus();
						ImGui::SetKeyboardFocusHere(-1); // Set focus to the text field.
					}

					if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
					{
						ImGui::SetKeyboardFocusHere(-1);
					}

					popupSize = ImVec2(ImGui::GetItemRectSize().x - 60, ImGui::GetTextLineHeightWithSpacing() * 2);
					popupPos = ImGui::GetItemRectMin();
					popupPos.y += ImGui::GetItemRectSize().y;
				}
				ImGui::End();

				// Draw popup autocomplete window
				if (m_Candidates.GetSize() > 0)
				{
					bool isActiveIndex = false;
					bool popupOpen = true;
					ImGuiWindowFlags popupFlags =
						ImGuiWindowFlags_NoTitleBar |
						ImGuiWindowFlags_NoResize |
						ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_HorizontalScrollbar |
						ImGuiWindowFlags_NoSavedSettings |
						ImGuiWindowFlags_NoFocusOnAppearing;

					if (m_Candidates.GetSize() < 10)
					{
						popupFlags |= ImGuiWindowFlags_NoScrollbar;
						popupSize.y = (ImGui::GetTextLineHeight() + 8) * m_Candidates.GetSize();
					}
					else
					{
						popupSize.y = (ImGui::GetTextLineHeight() + 8) * 10;
					}

					ImGui::SetNextWindowPos(popupPos);
					ImGui::SetNextWindowSize(popupSize);
					ImGui::Begin("candidates_popup", &popupOpen, popupFlags);
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
					ImGui::PushAllowKeyboardFocus(false);

					for (uint32 i = 0; i < m_Candidates.GetSize(); i++)
					{
						isActiveIndex = m_ActivePopupIndex == i;
						ImGui::PushID(i);
						if (ImGui::Selectable(m_Candidates[i].c_str(), &isActiveIndex))
						{
							PushError("Test");
						}
						ImGui::PopID();

						if (isActiveIndex && m_PopupSelectionChanged)
						{
							ImGui::SetScrollHere();
							m_PopupSelectedText = m_Candidates[i];
							m_PopupSelectionChanged = false;
						}
					}

					ImGui::PopAllowKeyboardFocus();
					ImGui::PopStyleVar();
					ImGui::End();
				}
			});
	}

	void GameConsole::BindCommand(ConsoleCommand cmd, std::function<void(CallbackInput&)> callback)
	{
		m_CommandMap[cmd.GetName()] = std::pair<ConsoleCommand, std::function<void(CallbackInput&)>>(cmd, callback);
	}

	GameConsole& GameConsole::Get()
	{
		static GameConsole console;
		return console;
	}

	GameConsole::GameConsole()
	{
	}

	GameConsole::~GameConsole()
	{
	}

	int GameConsole::ExecCommand(std::string& data)
	{
		std::string command = data;
		m_History.PushBack(command);

		Item item = {};
		item.Str = data;
		item.Color = glm::vec4(1.f, 1.f, 1.f, 1.f);
		m_Items.PushBack(item);

		m_ScrollToBottom = true;

		size_t cmdPos = command.find(" ");
		size_t pos = cmdPos != std::string::npos ? cmdPos : command.length();
		std::string token = command.substr(0, pos);
		command.erase(0, pos + std::string(" ").length());
		
		auto it = m_CommandMap.find(token);
		if (it == m_CommandMap.end())
		{
			PushError("Command '" + token + "' not found.");
			return 0;
		}

		ConsoleCommand& cmd = it->second.first;
		#ifndef LAMBDA_DEBUG
			if (cmd.IsDebug())
			{
				PushError("Command '" + token + "' not found.");
				return 0;
			}
		#endif
		Flag* preFlag = nullptr;
		std::unordered_map<std::string, Flag> flags;

		bool wasFlag = false;
		uint32 argIndex = 0;
		while (((pos = command.find(" ")) != std::string::npos) || ((pos = command.length()) > 0))
		{
			Arg arg;
			token = command.substr(0, pos);
			if (std::regex_match(token, std::regex("-[[:alpha:]].*")))
			{
				wasFlag = true;
				Flag flag;
				flag.Name = token.substr(1);

				if (cmd.GetFlags().find(flag.Name) == cmd.GetFlags().end())
				{
					PushError("'" + token + "' is an invalid flag!");
					return 0;
				}

				flags[flag.Name] = flag;
				preFlag = &flags[flag.Name];
			}
			else
			{
				FillArg(arg, token);
				if (wasFlag)
				{
					if (cmd.GetFlags()[preFlag->Name].Arg.Type != Arg::EType::EMPTY)
					{
						preFlag->Arg = arg;
					}
					else
					{
						bool res = AddArg(argIndex, arg, cmd);
						if (!res) return 0;
						argIndex++;
					}
					wasFlag = false;
				}
				else
				{
					bool res = AddArg(argIndex, arg, cmd);
					if (!res) return 0;
					argIndex++;
				}
			}

			if (std::string::npos != command.find(" "))
				command.erase(0, pos + std::string(" ").length());
			else
				command.erase(0, pos);
		}

		uint32 size = cmd.GetArguments().GetSize();
		if (argIndex != size) // Error too few arguments!
		{
			PushError("Too few arguments!");
			return 0;
		}

		// Call function
		CallbackInput input;
		input.Arguments = cmd.GetArguments();
		input.Flags = flags;
		it->second.second(input);

		return 0;
	}

	void GameConsole::FillArg(Arg& arg, std::string token)
	{
		arg.Type = Arg::EType::STRING;
		strcpy(arg.Value.Str, token.c_str());

		if (std::regex_match(token, std::regex("-[0-9]+")))
		{
			arg.Type = Arg::EType::INT;
			arg.Value.I = std::stoi(token);
		}
		else if (std::regex_match(token, std::regex("[0-9]+")))
		{
			arg.Type = Arg::EType::INT;
			arg.Value.I = std::stoi(token);
		}
		else if (std::regex_match(token, std::regex("(-[0-9]*\\.[0-9]+)|(-[0-9]+\\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.F = std::stof(token);
		}
		else if (std::regex_match(token, std::regex("([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.F = std::stof(token);
		}
		std::for_each(token.begin(), token.end(), [](char& c) { c = (char)std::tolower(c); });
		if (std::regex_match(token, std::regex("(false)|(true)")))
		{
			arg.Type = Arg::EType::BOOL;
			arg.Value.B = token == "false" ? false : true;
		}
	}

	bool GameConsole::AddArg(uint32 index, Arg arg, ConsoleCommand& cmd)
	{
		uint32 size = cmd.GetArguments().GetSize();
		if (index >= size) // Error too many arguments
		{
			PushError("Too many arguments!");
			return false;
		}
		if (cmd.GetArguments()[index].Type != arg.Type) // Error wrong type
		{
			PushError("Wrong argument type!");
			return false;
		}
		cmd.GetArguments()[index].Value = arg.Value;
		return true;
	}

	void GameConsole::PushError(const std::string& msg)
	{
		PushMsg("Error:" + msg, glm::vec4(1.f, 0.f, 0.f, 1.f));
	}

	void GameConsole::PushInfo(const std::string& msg)
	{
		PushMsg(msg, glm::vec4(1.f, 1.f, 0.f, 1.f));
	}

	void GameConsole::PushMsg(const std::string& line, glm::vec4 color)
	{
		Item item = {};
		item.Str = line;
		item.Color = color;
		m_Items.PushBack(item);
		m_ScrollToBottom = true;
	}

	int GameConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackEdit:
		{
			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			m_Candidates.Clear();
			for (auto& cmd : m_CommandMap)
			{
				const char* command = cmd.first.c_str();
				int32 d = -1;
				int32 n = (int32)(word_end - word_start);
				const char* s1 = command;
				const char* s2 = word_start;
				while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
				{
					s1++;
					s2++;
					n--;
				}
				if (d == 0)
				{
					m_Candidates.PushBack(command);
				}
			}

			break;
		}
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			TArray<const char*> candidates;
			//m_Candidates.Clear();
			for (auto& cmd : m_CommandMap)
			{
				const char* command = cmd.first.c_str();
				int32 d = 0;
				int32 n = (int32)(word_end - word_start);
				const char* s1 = command;
				const char* s2 = word_start;
				while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
				{
					s1++;
					s2++;
					n--;
				}
				if (d == 0)
				{
					candidates.PushBack(command);
				}
			}

			if (candidates.GetSize() == 0)
			{
				// No match
				m_ActivePopupIndex = -1;
			}
			else if (candidates.GetSize() == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else if (m_ActivePopupIndex != -1)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, m_PopupSelectedText.c_str());
				data->InsertChars(data->CursorPos, " ");
				m_ActivePopupIndex = -1;
				m_PopupSelectedText = "";
				m_Candidates.Clear();
			}
			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			if (m_Candidates.GetSize() == 0)
			{
				// Show history when nothing is typed (no candidates)
				const int prevHistoryIndex = m_HistoryIndex;
				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (m_HistoryIndex == -1)
						m_HistoryIndex = m_History.GetSize() - 1;
					else if (m_HistoryIndex > 0)
						m_HistoryIndex--;
				}
				else if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (m_HistoryIndex != -1)
						if (++m_HistoryIndex >= m_History.GetSize())
							m_HistoryIndex = -1;
				}

				if (prevHistoryIndex != m_HistoryIndex)
				{
					const char* historyStr = (m_HistoryIndex >= 0) ? m_History[m_HistoryIndex].c_str() : "";
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, historyStr);
				}
			}
			else
			{
				// Navigate candidates list
				if (data->EventKey == ImGuiKey_UpArrow && m_ActivePopupIndex > 0)
				{
					m_ActivePopupIndex--;
					m_PopupSelectionChanged = true;
				}
				else if (data->EventKey == ImGuiKey_DownArrow && m_ActivePopupIndex < ((int32)(m_Candidates.GetSize()) - 1))
				{
					m_ActivePopupIndex++;
					m_PopupSelectionChanged = true;
				}
			}
			break;
		}
		}
		return 0;
	}
}
