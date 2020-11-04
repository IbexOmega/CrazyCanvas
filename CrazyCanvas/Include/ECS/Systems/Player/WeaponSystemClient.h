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

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

	virtual void Fire(
		EAmmoType ammoType,
		LambdaEngine::Entity weaponOwner,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& playerPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity) override final;

protected:
	virtual bool InitInternal() override final;

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

	virtual bool TryFire(
		EAmmoType ammoType,
		WeaponComponent& weaponComponent,
		PacketComponent<PacketPlayerAction>& packets,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& startPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity) override final;

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