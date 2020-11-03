#pragma once

#include "ECS/System.h"

class ReplayBaseSystem : public LambdaEngine::System
{
	friend class ReplaySystem;

public:
	virtual ~ReplayBaseSystem() = default;

	virtual void Init() = 0;

	virtual void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) = 0;

protected:
	virtual void PlaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, int32 simulationTick) = 0;
	virtual void ReplaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, uint32 i, int32 simulationTick) = 0;
	virtual void SurrenderGameState(int32 simulationTick) = 0;
	virtual bool CompareNextGamesStates(int32 simulationTick) = 0;
	virtual int32 GetNextAvailableSimulationTick() = 0;
	virtual void DeleteGameState(int32 simulationTick) = 0;

	virtual void RegisterSystem(const LambdaEngine::String& systemName, LambdaEngine::SystemRegistration& systemRegistration) override;

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };
};
