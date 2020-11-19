#pragma once

#include "ECS/System.h"

class ReplayBaseSystem : public LambdaEngine::System
{
	friend class ReplayManagerSystem;

public:
	DECL_INTERFACE(ReplayBaseSystem);

protected:
	void RegisterSystem(const LambdaEngine::String& systemName, LambdaEngine::SystemRegistration& systemRegistration) override;

private:
	virtual void PlaySimulationTick(float32 dt, int32 simulationTick) = 0;
	virtual void SurrenderGameState(int32 simulationTick) = 0;
	virtual void ReplaySimulationTick(float32 dt, uint32 i, int32 simulationTick) = 0;
	virtual bool CompareNextGamesStates(int32 simulationTick) = 0;
	virtual void DeleteGameState(int32 simulationTick) = 0;
	virtual int32 GetNextAvailableSimulationTick() = 0;
};

template<class C, class S>
class ReplaySystem : public ReplayBaseSystem
{
public:
	virtual ~ReplaySystem() = default;

	virtual void Init() = 0;

protected:
	void RegisterServerGameStates(const LambdaEngine::TArray<S>& gameStates);
	virtual void PlaySimulationTick(float32 dt, C& clientState) = 0;
	virtual void ReplayGameState(float32 dt, C& clientState) = 0;
	virtual void SurrenderGameState(const S& serverState) = 0;
	virtual bool CompareGamesStates(const C& clientState, const S& serverState) = 0;

private:
	void PlaySimulationTick(float32 dt, int32 simulationTick) override;
	void SurrenderGameState(int32 simulationTick) override;
	void ReplaySimulationTick(float32 dt, uint32 i, int32 simulationTick) override;
	bool CompareNextGamesStates(int32 simulationTick) override;
	void DeleteGameState(int32 simulationTick) override;
	int32 GetNextAvailableSimulationTick() override;

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };

private:
	LambdaEngine::TArray<C> m_FramesToReconcile;
	LambdaEngine::TArray<S> m_FramesProcessedByServer;
};

template<class C, class S>
void ReplaySystem<C, S>::RegisterServerGameStates(const LambdaEngine::TArray<S>& gameStates)
{
	m_FramesProcessedByServer.Insert(m_FramesProcessedByServer.end(), gameStates.begin(), gameStates.end());
}

template<class C, class S>
void ReplaySystem<C, S>::PlaySimulationTick(float32 dt, int32 simulationTick)
{
	C clientState;
	clientState.SimulationTick = simulationTick;
	PlaySimulationTick(dt, clientState);
	m_FramesToReconcile.PushBack(clientState);
}

template<class C, class S>
void ReplaySystem<C, S>::SurrenderGameState(int32 simulationTick)
{
	const S& serverState = m_FramesProcessedByServer[0];

	ASSERT(m_FramesToReconcile[0].SimulationTick == simulationTick);
	ASSERT(serverState.SimulationTick == simulationTick);

	SurrenderGameState(serverState);
}

template<class C, class S>
void ReplaySystem<C, S>::ReplaySimulationTick(float32 dt, uint32 i, int32 simulationTick)
{
	C& clientState = m_FramesToReconcile[i];

	ASSERT(clientState.SimulationTick == simulationTick);

	ReplayGameState(dt, clientState);
}

template<class C, class S>
bool ReplaySystem<C, S>::CompareNextGamesStates(int32 simulationTick)
{
	const C& clientState = m_FramesToReconcile[0];
	const S& serverState = m_FramesProcessedByServer[0];

	ASSERT(clientState.SimulationTick == simulationTick);
	ASSERT(serverState.SimulationTick == simulationTick);

	return CompareGamesStates(clientState, serverState);
}

template<class C, class S>
void ReplaySystem<C, S>::DeleteGameState(int32 simulationTick)
{
	ASSERT(m_FramesToReconcile[0].SimulationTick == simulationTick);
	ASSERT(m_FramesProcessedByServer[0].SimulationTick == simulationTick);

	m_FramesToReconcile.Erase(m_FramesToReconcile.begin());
	m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.begin());
}

template<class C, class S>
int32 ReplaySystem<C, S>::GetNextAvailableSimulationTick()
{
	if (m_FramesProcessedByServer.IsEmpty())
		return -1;

	return m_FramesProcessedByServer[0].SimulationTick;
}