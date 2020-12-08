#include "World/Player/Client/PlayerSoundHelper.h"

#include "Resources/ResourceManager.h"

void PlayerSoundHelper::HandleMovementSound(
	const LambdaEngine::VelocityComponent& velocityComponent,
	LambdaEngine::AudibleComponent& audibleComponent,
	const glm::i8vec3& deltaAction,
	bool walking,
	bool inAir)
{
	using namespace LambdaEngine;

	bool horizontalAction = deltaAction.x != 0 || deltaAction.z != 0;
	bool verticalAction = deltaAction.y != 0;
	bool verticalMovement = glm::epsilonNotEqual<float32>(velocityComponent.Velocity.y, 0.0f, glm::epsilon<float32>());

	if (!inAir)
	{
		if (!walking && horizontalAction && !verticalMovement)
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

		if (verticalAction)
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
	}
}
