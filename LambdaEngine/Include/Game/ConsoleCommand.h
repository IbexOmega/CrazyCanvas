#pragma once

namespace LambdaEngine
{
	struct LAMBDA_API Arg
	{
		enum EType { FLOAT, BOOL, INT, STRING };

		EType type;
		union Value
		{
			float f;
			bool b;
			int i;
			char str[64];
		} value;
	};

	class LAMBDA_API ConsoleCommand
	{
	public:
		ConsoleCommand();
		~ConsoleCommand() = default;

		void Init(std::string name, bool isDebug);
		void AddArg(Arg& arg);

		std::string GetName() const;
		bool IsDebug() const;
		TArray<Arg>& GetArguments();

	private:
		std::string m_Name;
		bool m_IsDebug;
		TArray<Arg> m_Arguments;
	};
}