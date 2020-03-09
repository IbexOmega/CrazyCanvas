#include "LambdaEngine.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	bool EngineLoop::PreInit()
	{
		return true;
	}
	
	bool EngineLoop::Init()
	{
		return true;
	}
	
	bool EngineLoop::Run()
	{
		return true;
	}
	
	bool EngineLoop::Release()
	{
		return true;
	}
	
	bool EngineLoop::PostRelease()
	{
		return true;
	}

	void EngineLoop::Tick()
	{
	}
}