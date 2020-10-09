#include "Game/ConsoleCommand.h"

namespace LambdaEngine
{
	ConsoleCommand::ConsoleCommand() : m_Name(""), m_IsDebug(false)
	{
	}

	void ConsoleCommand::Init(const std::string& name, bool isDebug)
	{
		m_Name = name;
		m_IsDebug = isDebug;
	}

	void ConsoleCommand::AddArg(Arg::EType type)
	{
		Arg arg;
		arg.Type = type;
		m_Arguments.PushBack(arg);
	}

	void ConsoleCommand::AddArray(Arg::EType type, uint32 count)
	{
		for (uint32 i = 0; i < count; i++)
		{
			Arg arg;
			arg.Type = type;
			arg.IsArray = true;
			m_Arguments.PushBack(arg);
		}
	}

	void ConsoleCommand::AddFlag(const std::string& name, Arg::EType type)
	{
		Flag flag;
		flag.Name = name;
		flag.Arg.Type = type;
		m_Flags[name] = flag;
	}

	void ConsoleCommand::AddFlag(const std::string& name, Arg::EType type, uint32 count)
	{
		Flag flag;
		flag.Name = name;
		for (uint32 i = 0; i < count; i++)
		{
			Arg arg;
			arg.Type = type;
			arg.IsArray = true;
			flag.Args.PushBack(arg);
		}
		m_Flags[name] = flag;
	}

	void ConsoleCommand::AddDescription(const std::string& mainDescription)
	{
		m_Description.MainDesc = mainDescription;
	}

	void ConsoleCommand::AddDescription(const std::string& mainDescription, const std::unordered_map<std::string, std::string>& flagDescriptions)
	{
		m_Description.MainDesc = mainDescription;
		m_Description.FlagDescs = flagDescriptions;
	}

	std::string ConsoleCommand::GetName() const
	{
		return m_Name;
	}

	ConsoleCommand::Description ConsoleCommand::GetDescription() const
	{
		return m_Description;
	}

	bool ConsoleCommand::IsDebug() const
	{
		return m_IsDebug;
	}

	TArray<Arg>& ConsoleCommand::GetArguments()
	{
		return m_Arguments;
	}

	std::unordered_map<std::string, Flag>& ConsoleCommand::GetFlags()
	{
		return m_Flags;
	}
}
