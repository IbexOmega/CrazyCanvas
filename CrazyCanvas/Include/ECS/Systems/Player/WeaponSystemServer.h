#pragma once
#include "WeaponSystem.h"

/*
* WeaponSystemServer
*/

class WeaponSystemServer : public WeaponSystem
{
public:
	WeaponSystemServer() = default;
	~WeaponSystemServer() = default;

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

protected:
	LambdaEngine::IDVector m_RemotePlayerEntities;
};