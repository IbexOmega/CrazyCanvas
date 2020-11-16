#pragma once
#include "HealthSystem.h"

#include "Threading/API/SpinLock.h"

/*
* HealthSystemServer 
*/

class HealthSystemServer : public HealthSystem
{
public:
	HealthSystemServer() = default;
	~HealthSystemServer() = default;

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

private:
	LambdaEngine::IDVector m_MeshPaintEntities;
};