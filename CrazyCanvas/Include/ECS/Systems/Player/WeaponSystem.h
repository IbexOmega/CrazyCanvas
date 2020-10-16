#pragma once

#include "ECS/System.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct CollisionInfo;
}

struct WeaponComponent;

class WeaponSystem : public LambdaEngine::System
{
public:
	WeaponSystem() = default;
	~WeaponSystem() = default;

	bool Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	void Fire(WeaponComponent& weaponComponent, const glm::vec3& startPos, const glm::quat& direction, const glm::vec3& playerVelocity);
	void OnProjectileHit(const LambdaEngine::CollisionInfo& collisionInfo, LambdaEngine::Entity entity);

private:
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_PlayerEntities;

	// Rendering resources for projectiles
	LambdaEngine::MeshComponent m_ProjectileMeshComponent;
};
