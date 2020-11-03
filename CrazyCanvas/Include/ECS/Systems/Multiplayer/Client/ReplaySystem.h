#pragma once

#include "ECS/System.h"

class ReplayBaseSystem;

class ReplaySystem : public LambdaEngine::System
{
	friend class ReplayBaseSystem;

public:
	ReplaySystem();
	~ReplaySystem();

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
	static ReplaySystem* GetInstance() { return s_pInstance; }

private:
	int32 m_SimulationTick;
	int32 m_SimulationTickApproved;
	LambdaEngine::IDVector m_PlayerLocalEntities;
	LambdaEngine::TArray<ReplayBaseSystem*> m_ReplaySystems;

private:
	static ReplaySystem* s_pInstance;
};
