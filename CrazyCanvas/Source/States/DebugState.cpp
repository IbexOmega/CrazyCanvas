#include "States/DebugState.h"
#include "Log/Log.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Physics/Transform.h"
using namespace LambdaEngine;

DebugState::DebugState()
{

}

DebugState::DebugState(LambdaEngine::State* pOther) : LambdaEngine::State(pOther)
{
}

DebugState::~DebugState()
{
	// Remove System
}

void DebugState::Init()
{
	// Create Systems
	
	//Entity e = ECSCore::GetInstance()->CreateEntity();

	// Load Scene SceneManager::Get("SceneName").Load()

	// Use HelperClass to create additional entities

	// EntityIndex index = HelperClass::CreatePlayer(
	
	// Entity e = ECSCore::Get().createEntity()
}

void DebugState::Resume()
{
	// Unpause System

	// Reload Page
}

void DebugState::Pause()
{
	// Pause System

	// Unload Page
}

void DebugState::Tick(float dt)
{
	// Update State specfic objects
}
