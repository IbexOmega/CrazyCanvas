#include "Game/ConsoleCommand.h"

LambdaEngine::ConsoleCommand::ConsoleCommand() : m_Name(""), m_IsDebug(false)
{
}

void LambdaEngine::ConsoleCommand::Init(std::string name, bool isDebug)
{
	m_Name = name;
	m_IsDebug = isDebug;
}

void LambdaEngine::ConsoleCommand::AddArg(Arg::EType type)
{
	Arg arg;
	arg.type = type;
	m_Arguments.PushBack(arg);
}

void LambdaEngine::ConsoleCommand::AddFlag(const std::string& name, Arg::EType type)
{
	Flag flag;
	flag.name = name;
	flag.arg.type = type;
	m_Flags[name] = flag;
}

void LambdaEngine::ConsoleCommand::AddDescription(const std::string& mainDescription)
{
	m_Description.mainDesc = mainDescription;
}

void LambdaEngine::ConsoleCommand::AddDescription(const std::string& mainDescription, std::unordered_map<std::string, std::string> flagDescriptions)
{
	m_Description.mainDesc = mainDescription;
	m_Description.flagDescs = flagDescriptions;
}

std::string LambdaEngine::ConsoleCommand::GetName() const
{
	return m_Name;
}

LambdaEngine::ConsoleCommand::Description LambdaEngine::ConsoleCommand::GetDescription() const
{
	return m_Description;
}

bool LambdaEngine::ConsoleCommand::IsDebug() const
{
	return m_IsDebug;
}

LambdaEngine::TArray<LambdaEngine::Arg>& LambdaEngine::ConsoleCommand::GetArguments()
{
	return m_Arguments;
}

std::unordered_map<std::string, LambdaEngine::Flag>& LambdaEngine::ConsoleCommand::GetFlags()
{
	return m_Flags;
}
