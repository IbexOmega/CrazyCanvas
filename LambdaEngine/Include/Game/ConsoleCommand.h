#pragma once

namespace LambdaEngine
{
	struct LAMBDA_API Arg
	{
		enum class EType { FLOAT, BOOL, INT, STRING, EMPTY };

		EType Type;
		union Value
		{
			float F;
			bool B;
			int I;
			char Str[64];
		} Value;
	};

	struct LAMBDA_API Flag
	{
		Arg Arg;
		std::string Name;
	};

	class LAMBDA_API ConsoleCommand
	{
	private:
		struct Description
		{
			std::string MainDesc;
			std::unordered_map<std::string, std::string> FlagDescs;
		};

	public:
		ConsoleCommand();
		~ConsoleCommand() = default;

		void Init(std::string name, bool isDebug);
		void AddArg(Arg::EType type);
		void AddFlag(const std::string& name, Arg::EType type);

		void AddDescription(const std::string& mainDescription);
		void AddDescription(const std::string& mainDescription, std::unordered_map<std::string, std::string> flagDescriptions);

		std::string GetName() const;
		Description GetDescription() const;
		bool IsDebug() const;
		TArray<Arg>& GetArguments();
		std::unordered_map<std::string, Flag>& GetFlags();

	private:
		friend class GameConsole;

		std::string m_Name;
		Description m_Description;
		bool m_IsDebug;
		TArray<Arg> m_Arguments;
		std::unordered_map<std::string, Flag> m_Flags;
	};
}