#include "World/Player/Client/PlayerSoundHelper.h"

#include "Resources/ResourceManager.h"

void PlayerSoundHelper::HandleMovementSound(
	const LambdaEngine::PositionComponent& positionComponent,
	const LambdaEngine::VelocityComponent& velocityComponent,
	LambdaEngine::AudibleComponent& audibleComponent,
	const glm::i8vec3& deltaAction,
	bool walking,
	bool inAir)
{
	using namespace LambdaEngine;

	ISoundInstance3D* pStepSound = nullptr;

	if (auto stepSoundInstanceIt = audibleComponent.SoundInstances3D.find("Step"); stepSoundInstanceIt != audibleComponent.SoundInstances3D.end())
	{
		pStepSound = stepSoundInstanceIt->second;
	}
	else
	{
		LOG_ERROR("[PlayerSoundHelper]: Sound effect for Step could not be found in player audible component");
		return;
	}

	bool horizontalMovement = deltaAction.x != 0 || deltaAction.z != 0;
	bool verticalMovement = glm::epsilonNotEqual<float32>(velocityComponent.Velocity.y, 0.0f, glm::epsilon<float32>());

	if (!walking && horizontalMovement && !verticalMovement && !inAir)
	{
		pStepSound->Play();
	}
	else
	{
		pStepSound->Stop();
	}
}
