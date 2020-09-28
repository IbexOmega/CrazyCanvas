#pragma once
#include "LambdaEngine.h"
#include "ConsoleCommand.h"

#include "Math/Math.h"

#include "Application/API/Events/KeyEvents.h"

#include <unordered_map>

struct ImGuiInputTextCallbackData;
namespace LambdaEngine
{
	class LAMBDA_API GameConsole
	{
	public:
		struct CallbackInput
		{
			TArray<Arg> Arguments;
			std::unordered_map<std::string, Flag> Flags;
		};

	public:
		DECL_REMOVE_COPY(GameConsole);
		DECL_REMOVE_MOVE(GameConsole);

		bool Init();
		static bool Release();

		void Tick();

		void BindCommand(ConsoleCommand cmd, std::function<void(CallbackInput&)> callback);

		static GameConsole& Get();

	private:
		GameConsole();
		~GameConsole();

		struct Item
		{
			std::string Str;
			glm::vec4 Color;
		};

		bool InitCommands();
		
		int ExecCommand(const std::string& data);

		static void FillArg(Arg& arg, std::string token);

		bool AddArg(uint32 index, Arg arg, ConsoleCommand& cmd);

		void PushError(const std::string& msg);
		void PushInfo(const std::string& msg);
		void PushMsg(const std::string& line, glm::vec4 color);

		bool OnKeyPressed(const KeyPressedEvent& event);

		int TextEditCallback(ImGuiInputTextCallbackData* data);

	private:
		TArray<std::string> m_Candidates;
		TArray<Item> m_Items;
		TArray<std::string> m_History;
		
		int32 m_HistoryIndex { -1 };

		char m_Buf[256];

		bool m_ScrollToBottom = false;
		bool m_IsActive = false;
		
		int32 m_ActivePopupIndex = -1;
		bool m_PopupSelectionChanged = false;
		bool m_UpdatePositionCursor = false;
		
		std::string m_PopupSelectedText = "";
		
		std::unordered_map<std::string, std::pair<ConsoleCommand, std::function<void(CallbackInput&)>>> m_CommandMap;
	};
}
