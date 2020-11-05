#pragma once
#include "HealthSystem.h"

/*
* HealthSystemClient
*/

class HealthSystemClient : public HealthSystem
{
public:
	HealthSystemClient() = default;
	~HealthSystemClient() = default;

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

private:
	LambdaEngine::IDVector m_LocalPlayerEntities;
};