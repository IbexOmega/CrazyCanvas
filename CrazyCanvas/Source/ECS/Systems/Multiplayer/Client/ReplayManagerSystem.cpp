#include "ECS/Systems/Multiplayer/Client/ReplayManagerSystem.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"

#include "Math/Math.h"

#include "Engine/EngineLoop.h"

using namespace LambdaEngine;

ReplayManagerSystem* ReplayManagerSystem::s_pInstance = nullptr;

ReplayManagerSystem::ReplayManagerSystem() : 
	m_SimulationTick(0),
	m_SimulationTickApproved(-1),
	m_ReplaySystems()
{
	ASSERT(s_pInstance == nullptr);
	s_pInstance = this;
}

ReplayManagerSystem::~ReplayManagerSystem()
{
	s_pInstance = nullptr;
}

void ReplayManagerSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_PlayerLocalEntities,
			.ComponentAccesses =
			{
				{NDA, PlayerLocalComponent::Type()}
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(TYPE_NAME(ReplayManagerSystem), systemReg);
}

void ReplayManagerSystem::FixedTickMainThread(Timestamp deltaTime)
{
	if (m_PlayerLocalEntities.Empty())
		return;

	float32 dt = (float32)deltaTime.AsSeconds();

	for (ReplayBaseSystem* pSystem : m_ReplaySystems)
	{
		pSystem->PlaySimulationTick(deltaTime, dt, m_SimulationTick);
	}

	while (true)
	{
		int32 oldestAvailableSimulationTick = INT32_MAX;

		for (ReplayBaseSystem* pSystem : m_ReplaySystems)
		{
			int32 simTick = pSystem->GetNextAvailableSimulationTick();
			if (simTick < oldestAvailableSimulationTick)
				oldestAvailableSimulationTick = simTick;
		}

		if (IsNewTickToCompare(oldestAvailableSimulationTick))
		{
			bool predictionError = CheckForPredictionError(oldestAvailableSimulationTick);
			DeleteGameState(oldestAvailableSimulationTick);

			if (predictionError)
				ReplaySimulationTicksFrom(oldestAvailableSimulationTick + 1);

			m_SimulationTickApproved = oldestAvailableSimulationTick;
		}
		else
		{
			break;
		}
	}
	
	m_SimulationTick++;
}

void ReplayManagerSystem::ReplaySimulationTicksFrom(int32 simulationTick)
{
	Timestamp deltaTime = EngineLoop::GetFixedTimestep();
	float32 dt = (float32)deltaTime.AsSeconds();

	uint32 index = 0;
	for (int32 tick = simulationTick; tick <= m_SimulationTick; tick++)
	{
		for (ReplayBaseSystem* pSystem : m_ReplaySystems)
		{
			pSystem->ReplaySimulationTick(deltaTime, dt, index, tick);
		}
		index++;
	}
}

void ReplayManagerSystem::RegisterReplaySystem(ReplayBaseSystem* pReplaySystem)
{
	m_ReplaySystems.PushBack(pReplaySystem);
}

bool ReplayManagerSystem::IsNewTickToCompare(int32 oldestAvailableSimulationTick)
{
	if (oldestAvailableSimulationTick >= 0 && oldestAvailableSimulationTick < INT32_MAX)
	{
		if (m_SimulationTickApproved + 1 == oldestAvailableSimulationTick)
		{
			return true;
		}
	}
	return false;
}

bool ReplayManagerSystem::CheckForPredictionError(int32 simulationTick)
{
	bool predictionError = false;
	for (ReplayBaseSystem* pSystem : m_ReplaySystems)
	{
		if (!pSystem->CompareNextGamesStates(simulationTick))
		{
			predictionError = true;
			pSystem->SurrenderGameState(simulationTick);
		}
	}
	return predictionError;
}

void ReplayManagerSystem::DeleteGameState(int32 simulationTick)
{
	for (ReplayBaseSystem* pSystem : m_ReplaySystems)
	{
		pSystem->DeleteGameState(simulationTick);
	}
}
