#pragma once
#include "HealthSystem.h"

#include "Events/PlayerEvents.h"

/*
* HealthSystemClient
*/

class HealthSystemClient : public HealthSystem
{
public:
	HealthSystemClient() = default;
	~HealthSystemClient();

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

private:
	LambdaEngine::IDVector m_LocalPlayerEntities;
};