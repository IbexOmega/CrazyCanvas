#include "Game/ConsoleCommand.h"

LambdaEngine::ConsoleCommand::ConsoleCommand() : m_Name(""), m_IsDebug(false)
{
}

void LambdaEngine::ConsoleCommand::Init(std::string name, bool isDebug)
{
	m_Name = name;
	m_IsDebug = isDebug;
}

void LambdaEngine::ConsoleCommand::AddArg(Arg& arg)
{
	m_Arguments.PushBack(arg);
}

std::string LambdaEngine::ConsoleCommand::GetName() const
{
	return m_Name;
}

bool LambdaEngine::ConsoleCommand::IsDebug() const
{
	return m_IsDebug;
}

LambdaEngine::TArray<LambdaEngine::Arg>& LambdaEngine::ConsoleCommand::GetArguments()
{
	return m_Arguments;
}
