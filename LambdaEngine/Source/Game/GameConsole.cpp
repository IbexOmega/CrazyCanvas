#include "Game/GameConsole.h"

#include "Input/API/Input.h"

#include "Rendering/ImGuiRenderer.h"

#include "Application/API/CommonApplication.h"

#include "Application/API/Events/EventQueue.h"

#include <regex>
#include <imgui.h>

namespace LambdaEngine
{
	bool GameConsole::Init()
	{
		if (!InitCommands())
		{
			return false;
		}

		return EventQueue::RegisterEventHandler(this, &GameConsole::OnKeyPressed);
	}

	bool GameConsole::InitCommands()
	{
		ConsoleCommand cmdHelp;
		cmdHelp.Init("help", false);
		cmdHelp.AddFlag("d", Arg::EType::EMPTY);
		cmdHelp.AddDescription("Shows all commands descriptions.", { {"d", "Used to show debug commands also."} });
		BindCommand(cmdHelp, [this](CallbackInput& input)->void
		{
			for (auto i : m_CommandMap)
			{
				ConsoleCommand& cCmd = i.second.first;
				if (!cCmd.IsDebug() || (input.Flags.find("d") != input.Flags.end()))
				{
					ConsoleCommand::Description desc = cCmd.GetDescription();
					PushInfo(i.first + ":");
					PushInfo("\t" + desc.MainDesc);
					for (auto flagDesc : desc.FlagDescs)
						PushInfo("\t\t-" + flagDesc.first + ": " + flagDesc.second);
				}
			}
		});

		ConsoleCommand cmdClear;
		cmdClear.Init("clear", false);
		cmdClear.AddFlag("h", Arg::EType::EMPTY);
		cmdClear.AddDescription("Clears the visible text in the console.", { {"h", "Clears the history."} });
		BindCommand(cmdClear, [this](CallbackInput& input)->void
		{
			m_Items.Clear();
		});

		ConsoleCommand cmdExit;
		cmdExit.Init("exit", false);
		cmdExit.AddDescription("Terminate the application");
		BindCommand(cmdExit, [this](CallbackInput& input)->void
		{
			CommonApplication::Get()->Terminate();
		});

		ConsoleCommand cmdInput;
		cmdInput.Init("enable_input", false);
		cmdInput.AddArg(Arg::EType::BOOL);
		cmdInput.AddDescription("Enable or disable application input");
		BindCommand(cmdInput, [this](CallbackInput& input)->void
		{
			if (input.Arguments[0].Value.B)
			{
				Input::Enable();
			}
			else
			{
				Input::Disable();
			}
		});

		// Test Command
		ConsoleCommand cmd;
		cmd.Init("clo", true);
		cmd.AddArg(Arg::EType::STRING);
		cmd.AddFlag("l", Arg::EType::INT);
		cmd.AddFlag("i", Arg::EType::EMPTY);
		cmd.AddDescription("Does blah and do bar.");

		GameConsole::Get().BindCommand(cmd, [](GameConsole::CallbackInput& input)->void
		{
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
		return EventQueue::UnregisterEventHandler(this, &GameConsole::OnKeyPressed);
	}

	void GameConsole::Tick()
	{
		// Toggle console when pressing § (Button beneath Escape)
		static bool s_Active = false;
		static bool s_Toggle = false;

		// Do not draw if not active.
		if (!m_IsActive)
		{
			return;
		}

		ImGuiRenderer::Get().DrawUI([&]()
			{
				ImGuiWindowFlags flags = 
					ImGuiWindowFlags_NoMove | 
					ImGuiWindowFlags_NoTitleBar;

				TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
				uint32 width = mainWindow->GetWidth();
				uint32 height = mainWindow->GetHeight();
				const uint32 standardHeight = 200;

				// Draw a console window at the top right of the viewport.
				ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver); // Standard position
				ImGui::SetNextWindowSize(ImVec2(width, standardHeight), ImGuiCond_FirstUseEver); // Standard size
				ImGui::SetNextWindowSizeConstraints(ImVec2(width, 70), ImVec2(width, height)); // Window constraints
				
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f); // Make more transparent
				ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0); // Remove grip, resize works anyway
				ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0); // Remove grip, resize works anyway
				ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0); // Remove grip, resize works anyway

				if (ImGui::Begin("Console", (bool*)0, flags))
				{
					bool hasFocus = false;

					// History
					const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
					ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
					
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f); // Make less transparent
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

					// Only display visible text to see history.
					ImGuiListClipper clipper(m_Items.GetSize());
					while (clipper.Step())
					{
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
						{
							Item& item = m_Items[i];
							const char* str = item.Str.c_str();
							ImVec4 color = ImVec4(item.Color.r, item.Color.g, item.Color.b, item.Color.a);
							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::TextUnformatted(str);
							ImGui::PopStyleColor();
						}
					}

					if (m_ScrollToBottom | (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
					{
						ImGui::SetScrollHereY(0.0f);
					}

					m_ScrollToBottom = false;

					ImGui::PopStyleVar();
					ImGui::PopStyleVar();

					ImGui::EndChild();
					ImGui::Separator();

					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f); // Make less transparent

					// Command line
					static char s_Buf[256];
					
					ImGui::PushItemWidth(width);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.9f));

					ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;;
					if (ImGui::InputText("###Input", s_Buf, 256, input_text_flags, 
						[](ImGuiInputTextCallbackData* data)->int {
						GameConsole* console = (GameConsole*)data->UserData;
						return console->TextEditCallback(data);
						}, (void*)this))
					{
						if (s_Buf[0])
						{
							std::string buff = std::string(s_Buf);
							ExecCommand(buff);
						}

						strcpy(s_Buf, "");
						hasFocus = true;
					}

					ImGui::PopStyleColor();
					ImGui::PopItemWidth();
					ImGui::PopStyleVar();

					if (s_Active || hasFocus)
					{
						ImGui::SetItemDefaultFocus();
						ImGui::SetKeyboardFocusHere(-1); // Set focus to the text field.
					}

				}
				ImGui::End();

				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
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
		else if (std::regex_match(token, std::regex("(-[0-9]*\.[0-9]+)|(-[0-9]+\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.F = std::stof(token);
		}
		else if (std::regex_match(token, std::regex("([0-9]*\.[0-9]+)|([0-9]+\.[0-9]*)")))
		{
			arg.Type = Arg::EType::FLOAT;
			arg.Value.F = std::stof(token);
		}
		std::for_each(token.begin(), token.end(), [](char& c) { c = std::tolower(c); });
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
		Item item = {};
		item.Str = "Error:" + msg;
		item.Color = glm::vec4(1.f, 0.f, 0.f, 1.f);
		m_Items.PushBack(item);
		m_ScrollToBottom = true;
	}

	void GameConsole::PushInfo(const std::string& msg)
	{
		Item item = {};
		item.Str = msg;
		item.Color = glm::vec4(1.f, 1.f, 0.f, 1.f);
		m_Items.PushBack(item);
		m_ScrollToBottom = true;
	}

	bool GameConsole::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.Key == EKey::KEY_GRAVE_ACCENT)
		{
			m_IsActive = !m_IsActive;
		}

		return true;
	}

	int GameConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		switch (data->EventFlag)
		{
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
			}
			else if (candidates.GetSize() == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else
			{
				// Multiple matches. Complete as much as it can.
				int match_len = (int)(word_end - word_start);
				for (;;)
				{
					int c = 0;
					bool all_candidates_matches = true;
					for (int i = 0; i < candidates.GetSize() && all_candidates_matches; i++)
						if (i == 0)
							c = toupper(candidates[i][match_len]);
						else if (c == 0 || c != toupper(candidates[i][match_len]))
							all_candidates_matches = false;
					if (!all_candidates_matches)
						break;
					match_len++;
				}

				if (match_len > 0)
				{
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
					data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
				}

				// List matches
				PushInfo("Possible matches:\n");
				for (int i = 0; i < candidates.GetSize(); i++)
					PushInfo("-" + std::string(candidates[i]));
			}
			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
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
		}
		return 0;
	}
}
