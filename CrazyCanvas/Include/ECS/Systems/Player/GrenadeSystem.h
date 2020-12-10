#pragma once

#include "ECS/System.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct KeyPressedEvent;
}

namespace physx
{
	class PxShape;
}

class GrenadeSystem : LambdaEngine::System
{
public:
	GrenadeSystem() = default;
	~GrenadeSystem();

	bool Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	bool OnKeyPress(const LambdaEngine::KeyPressedEvent& keyPressEvent);

	// A player is attempting to throw a grenade. Throw one if the grenade is off cooldown.
	void TryThrowGrenade(LambdaEngine::Entity throwingPlayer);
	void ThrowGrenade(LambdaEngine::Entity throwingPlayer);

	void Explode(LambdaEngine::Entity grenade);

	/**
	 * @param players Players within the blast radius of the exploding grenade are pushed to this array
	*/
	void FindPlayersWithinBlast(LambdaEngine::TArray<LambdaEngine::Entity>& players, const glm::vec3& grenadePosition, uint8 grenadeTeam);

	/**
	 * Sends raycasts from the grenade to different places on players' bodies to figure out if they were hit.
	 * Should be called after finding the players within the blast radius of the grenade using the function above.
	 * @param players Players assumed to be within the blast radius of the grenade.
	*/
	void RaycastToPlayers(const LambdaEngine::TArray<LambdaEngine::Entity>& players, const glm::vec3& grenadePosition);

private:
	LambdaEngine::IDVector m_Grenades;
	LambdaEngine::IDVector m_ForeignPlayers;

	GUID_Lambda m_GrenadeMesh;
};
