#pragma once

#include "ECS/System.h"

class ReplayBaseSystem;

class ReplayManagerSystem : public LambdaEngine::System
{
	friend class ReplayBaseSystem;

public:
	ReplayManagerSystem();
	~ReplayManagerSystem();

	void Init();

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };

	void RegisterReplaySystem(ReplayBaseSystem* pReplaySystem);

	bool IsNewTickToCompare(int32 oldestAvailableSimulationTick);
	bool CheckForPredictionError(int32 simulationTick);
	void DeleteGameState(int32 simulationTick);
	void ReplaySimulationTicksFrom(int32 simulationTick);

private:
	static ReplayManagerSystem* GetInstance() { return s_pInstance; }

private:
	int32 m_SimulationTick;
	int32 m_SimulationTickApproved;
	LambdaEngine::IDVector m_PlayerLocalEntities;
	LambdaEngine::TArray<ReplayBaseSystem*> m_ReplaySystems;

private:
	static ReplayManagerSystem* s_pInstance;
};
