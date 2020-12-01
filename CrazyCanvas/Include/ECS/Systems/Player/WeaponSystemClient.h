#pragma once
#include "WeaponSystem.h"

#include "Events/PlayerEvents.h"


/*
* WeaponSystemClient
*/

class WeaponSystemClient : public WeaponSystem
{
public:
	WeaponSystemClient();
	~WeaponSystemClient();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;
	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

	virtual void Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint8 playerTeam, uint32 angle) override final;

	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

protected:
	virtual bool InitInternal() override final;

	bool TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity);

private:
	LambdaEngine::IDVector m_LocalPlayerEntities;
	LambdaEngine::IDVector m_ForeignPlayerEntities;
};