#include "Game/ECS/Systems/Rendering/AnimationSystem.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	AnimationSystem::AnimationSystem()
		: m_AnimationEntities()
	{
	}

	AnimationSystem::~AnimationSystem()
	{
	}

	void AnimationSystem::Animate(Timestamp deltaTime, AnimationComponent& animation, MeshComponent& mesh)
	{
		Animation* pAnimation = ResourceManager::GetAnimation(animation.AnimationGUID);
		VALIDATE(pAnimation);

		// Move timer
		animation.DurationInTicks += (pAnimation->TicksPerSecond * deltaTime.AsSeconds());
		if (animation.DurationInTicks > pAnimation->DurationInTicks)
		{
			animation.DurationInTicks = 0.0f;
		}

		// Find keyframes
		TArray<glm::mat4> boneMatrices;
		boneMatrices.Reserve(pAnimation->Channels.GetSize());

		for (Animation::Channel& channel : pAnimation->Channels)
		{
			// Interpolate position
			glm::vec3 position;
			{
				Animation::Channel::KeyFrame pos0;
				Animation::Channel::KeyFrame pos1;
				if (channel.Positions.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Positions.GetSize() - 1); i++)
					{
						if (channel.Positions[i].Time >= animation.DurationInTicks)
						{
							pos0 = channel.Positions[i];
							pos1 = channel.Positions[i + 1];
							break;
						}
					}
				}
				else
				{
					pos0 = channel.Positions[0];
					pos1 = channel.Positions[0];
				}

				position = glm::mix(pos0.Value, pos1.Value, glm::vec3(0.5f));
			}

			// Interpolate rotation
			glm::quat rotation;
			{
				Animation::Channel::RotationKeyFrame rot0;
				Animation::Channel::RotationKeyFrame rot1;
				if (channel.Rotations.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Rotations.GetSize() - 1); i++)
					{
						if (channel.Rotations[i].Time >= animation.DurationInTicks)
						{
							rot0 = channel.Rotations[i];
							rot1 = channel.Rotations[i + 1];
							break;
						}
					}
				}
				else
				{
					rot0 = channel.Rotations[0];
					rot1 = channel.Rotations[0];
				}

				rotation = glm::slerp(rot0.Value, rot1.Value, 0.5f);
			}

			// Interpolate scale
			glm::vec3 scale;
			{
				Animation::Channel::KeyFrame scale0;
				Animation::Channel::KeyFrame scale1;
				if (channel.Positions.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Scales.GetSize() - 1); i++)
					{
						if (channel.Scales[i].Time >= animation.DurationInTicks)
						{
							scale0 = channel.Scales[i];
							scale1 = channel.Scales[i + 1];
							break;
						}
					}
				}
				else
				{
					scale0 = channel.Scales[0];
					scale1 = channel.Scales[0];
				}

				scale = glm::mix(scale0.Value, scale1.Value, glm::vec3(0.5f));
			}

			glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), position);
			transform			= transform * glm::toMat4(rotation);
			transform			= glm::scale(transform, scale);
			boneMatrices.EmplaceBack(transform);
		}
	}

	bool AnimationSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{{{ RW, AnimationComponent::Type() }, { R, MeshComponent::Type() }},	&m_AnimationEntities,	std::bind(&AnimationSystem::OnEntityAdded, this, std::placeholders::_1), std::bind(&AnimationSystem::OnEntityRemoved, this, std::placeholders::_1) },
		};
		systemReg.Phase = 0;

		RegisterSystem(systemReg);

		return true;
	}

	void AnimationSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		ComponentArray<AnimationComponent>*	pAnimationComponents	= pECSCore->GetComponentArray<AnimationComponent>();
		ComponentArray<MeshComponent>*		pMeshComponents			= pECSCore->GetComponentArray<MeshComponent>();

		for (Entity entity : m_AnimationEntities.GetIDs())
		{
			MeshComponent&		mesh		= pMeshComponents->GetData(entity);
			AnimationComponent&	animation	= pAnimationComponents->GetData(entity);
			Animate(deltaTime, animation, mesh);
		}
	}

	void AnimationSystem::OnEntityAdded(Entity entity)
	{
		LOG_INFO("Animated Enity added");
	}

	void AnimationSystem::OnEntityRemoved(Entity entity)
	{
		LOG_INFO("Animated Enity removed");
	}
	
	AnimationSystem& AnimationSystem::GetInstance()
	{
		static AnimationSystem instance;
		return instance;
	}
}