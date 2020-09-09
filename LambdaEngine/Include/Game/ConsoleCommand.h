#pragma once

namespace LambdaEngine
{
	

	struct LAMBDA_API Arg
	{
		enum EType { FLOAT, BOOL, INT, STRING, EMPTY };

		EType type;
		union Value
		{
			float f;
			bool b;
			int i;
			char str[64];
		} value;
	};

	struct LAMBDA_API Flag
	{
		Arg arg;
		std::string name;
	};

	class LAMBDA_API ConsoleCommand
	{
	public:
		ConsoleCommand();
		~ConsoleCommand() = default;

		void Init(std::string name, bool isDebug);
		void AddArg(Arg::EType type);
		void AddFlag(const std::string& name, Arg::EType type);

		std::string GetName() const;
		bool IsDebug() const;
		TArray<Arg>& GetArguments();
		std::unordered_map<std::string, Flag>& GetFlags();

	private:
		std::string m_Name;
		bool m_IsDebug;
		TArray<Arg> m_Arguments;
		std::unordered_map<std::string, Flag> m_Flags;
	};
}