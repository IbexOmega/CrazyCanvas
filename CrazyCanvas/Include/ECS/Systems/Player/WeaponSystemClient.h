#pragma once
#include "WeaponSystem.h"

/*
* WeaponSystemClient
*/

class WeaponSystemClient : public WeaponSystem
{
public:
	WeaponSystemClient() = default;
	~WeaponSystemClient() = default;

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;
	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

	virtual void Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint8 playerTeam, uint32 angle) override final;

protected:
	virtual bool InitInternal() override final;

	bool TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity);

	virtual LambdaEngine::MeshComponent GetMeshComponent(EAmmoType ammoType, uint32 playerTeam) override final
	{
		if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			if (playerTeam == 0)
			{
				return m_BluePaintProjectileMeshComponent;
			}
			else
			{
				return m_RedPaintProjectileMeshComponent;
			}
		}
		else
		{
			return m_WaterProjectileMeshComponent;
		}
	}

private:
	LambdaEngine::IDVector m_LocalPlayerEntities;
	LambdaEngine::IDVector m_ForeignPlayerEntities;

	// Rendering resources for projectiles
	LambdaEngine::MeshComponent m_RedPaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_BluePaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_WaterProjectileMeshComponent;

	GUID_Lambda m_GunFireGUID	= GUID_NONE;
	GUID_Lambda m_OutOfAmmoGUID	= GUID_NONE;
};