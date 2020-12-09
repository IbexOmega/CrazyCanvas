#include "World/Player/Client/PlayerSoundHelper.h"

#include "Resources/ResourceManager.h"

void PlayerSoundHelper::HandleMovementSound(
	const LambdaEngine::VelocityComponent& velocityComponent,
	LambdaEngine::AudibleComponent& audibleComponent,
	bool walking,
	bool inAir,
	bool wasInAir)
{
	using namespace LambdaEngine;

	bool horizontalMovement = 
		glm::epsilonNotEqual<float32>(velocityComponent.Velocity.x, 0.0f, 0.5f) || 
		glm::epsilonNotEqual<float32>(velocityComponent.Velocity.z, 0.0f, 0.5f);
	bool verticalMovement = glm::epsilonNotEqual<float32>(velocityComponent.Velocity.y, 0.0f, 0.1f);

	if (!inAir)
	{
		if (!walking && horizontalMovement && !verticalMovement)
		{
			if (auto stepSoundInstanceIt = audibleComponent.SoundInstances3D.find("Step"); 
				stepSoundInstanceIt != audibleComponent.SoundInstances3D.end())
			{
				stepSoundInstanceIt->second->Play();
			}
			else
			{
				LOG_ERROR("[PlayerSoundHelper]: Sound effect for Step could not be found in player audible component");
				return;
			}
		}

		if (velocityComponent.Velocity.y > 4.0f)
		{
			if (auto stepSoundInstanceIt = audibleComponent.SoundInstances3D.find("Jump");
				stepSoundInstanceIt != audibleComponent.SoundInstances3D.end())
			{
				stepSoundInstanceIt->second->Play();
			}
			else
			{
				LOG_ERROR("[PlayerSoundHelper]: Sound effect for Jump could not be found in player audible component");
				return;
			}
		}

		if (wasInAir && velocityComponent.LastNonZeroVelocityComponents.y < -5.0f)
		{
			if (auto stepSoundInstanceIt = audibleComponent.SoundInstances3D.find("Landing");
				stepSoundInstanceIt != audibleComponent.SoundInstances3D.end())
			{
				stepSoundInstanceIt->second->Play();
			}
			else
			{
				LOG_ERROR("[PlayerSoundHelper]: Sound effect for Jump could not be found in player audible component");
				return;
			}
		}
	}
}
