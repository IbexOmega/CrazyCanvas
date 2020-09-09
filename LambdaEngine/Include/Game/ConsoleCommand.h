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
		Arg value;
		std::string name;
	};

	class LAMBDA_API ConsoleCommand
	{
	public:
		ConsoleCommand();
		~ConsoleCommand() = default;

		void Init(std::string name, bool isDebug);
		void AddArg(Arg& arg);
		void AddFlag(Flag& flag);

		std::string GetName() const;
		bool IsDebug() const;
		const TArray<Arg>& GetArguments() const;
		const TArray<Flag>& GetFlags() const;

	private:
		std::string m_Name;
		bool m_IsDebug;
		TArray<Arg> m_Arguments;
		TArray<Flag> m_Flags;
	};
}