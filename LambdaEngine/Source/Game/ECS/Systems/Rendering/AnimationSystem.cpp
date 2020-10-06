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

	void AnimationSystem::Animate(AnimationComponent& animation)
	{
		Animation* pAnimation = ResourceManager::GetAnimation(animation.AnimationGUID);
		VALIDATE(pAnimation);

		Skeleton* pSkeleton = animation.Pose.pSkeleton;
		VALIDATE(pSkeleton);

		// Make sure we have enough matrices
		if (animation.Pose.LocalTransforms.GetSize() < pSkeleton->Joints.GetSize())
		{
			animation.Pose.LocalTransforms.Resize(pSkeleton->Joints.GetSize(), glm::mat4(1.0f));
		}

		if (animation.Pose.GlobalTransforms.GetSize() < pSkeleton->Joints.GetSize())
		{
			animation.Pose.GlobalTransforms.Resize(pSkeleton->Joints.GetSize(), glm::mat4(1.0f));
		}

		// Get localtime for the clip
		float64 localTime = (GetTotalTimeInSeconds() - animation.StartTime) * fabs(animation.PlaybackSpeed);
		if (animation.IsLooping)
		{
			if (animation.NumLoops != INFINITE_LOOPS)
			{
				float64 totalDuration = animation.NumLoops * pAnimation->DurationInSeconds();
				localTime = glm::clamp(localTime, 0.0, totalDuration);
			}

			localTime = fmod(localTime, pAnimation->DurationInSeconds());
		}
		else
		{
			localTime = glm::clamp(localTime, 0.0, pAnimation->DurationInSeconds());
		}

		// Get normalized time
		const float64 normalizedLocalTime = localTime / pAnimation->DurationInSeconds();

		// Convert into ticks
		const float64 ticksPerSeconds = pAnimation->TicksPerSecond;
		float64 time = localTime * ticksPerSeconds;
		if (animation.PlaybackSpeed < 0.0)
		{
			time = pAnimation->DurationInTicks - time;
		}

		// Calculate SQT for each animation to blend between
		TArray<SQT> mainAnimation = CalculateSQT(*pAnimation, *pSkeleton, time, animation.IsLooping);
		
		TArray<SQT> blendAnimation;
		if (animation.BlendingAnimationGUID != GUID_NONE)
		{
			Animation* pBlendAnimation = ResourceManager::GetAnimation(animation.BlendingAnimationGUID);
			VALIDATE(pBlendAnimation);

			blendAnimation = CalculateSQT(*pBlendAnimation, *pSkeleton, time, animation.IsLooping);
		}

		for (SQT& sqt : mainAnimation)
		{
			glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), sqt.Translation);
			transform			= transform * glm::toMat4(sqt.Rotation);
			transform			= glm::scale(transform, sqt.Scale);

			// set transform
			const uint32 boneID = sqt.BoneID;
			animation.Pose.LocalTransforms[boneID] = transform;
		}
		
		// Calculate global transforms
		for (uint32 i = 0; i < pSkeleton->Joints.GetSize(); i++)
		{
			Joint& joint = pSkeleton->Joints[i];
			animation.Pose.GlobalTransforms[i] = pSkeleton->InverseGlobalTransform * ApplyParent(joint, *pSkeleton, animation.Pose.LocalTransforms) * joint.InvBindTransform;
		}
	}

	TArray<SQT> AnimationSystem::CalculateSQT(Animation& animation, Skeleton& skeleton, float64 time, bool isLooping)
	{
		TArray<SQT> sqt;
		sqt.Reserve(skeleton.Joints.GetSize());

		for (Animation::Channel& channel : animation.Channels)
		{
			// Retrive the bone ID
			auto it = skeleton.JointMap.find(channel.Name);
			if (it == skeleton.JointMap.end())
			{
				continue;
			}

			// Sample SQT for this animation
			glm::vec3 position	= SamplePosition(channel,	time, isLooping);
			glm::quat rotation	= SampleRotation(channel,	time, isLooping);
			glm::vec3 scale		= SampleScale(channel,		time, isLooping);
			sqt.EmplaceBack(it->second, position, scale, rotation);
		}

		return sqt;
	}

	glm::mat4 AnimationSystem::ApplyParent(Joint& bone, Skeleton& skeleton, TArray<glm::mat4>& matrices)
	{
		int32 parentID	= bone.ParentBoneIndex;
		int32 myID		= skeleton.JointMap[bone.Name];
		if (parentID == INVALID_JOINT_ID)
		{
			return matrices[myID];
		}

		return ApplyParent(skeleton.Joints[parentID], skeleton, matrices) * matrices[myID];
	}

	glm::vec3 AnimationSystem::SamplePosition(Animation::Channel& channel, float64 time, bool isLooping)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numPositions = isLooping ? channel.Positions.GetSize() - 1 : channel.Positions.GetSize();

		Animation::Channel::KeyFrame pos0 = channel.Positions[0];
		Animation::Channel::KeyFrame pos1 = channel.Positions[0];
		if (numPositions > 1)
		{
			for (uint32 i = 0; i < (numPositions - 1); i++)
			{
				if (time < channel.Positions[i + 1].Time)
				{
					pos0 = channel.Positions[i];
					pos1 = channel.Positions[i + 1];
					break;
				}
			}
		}

		const float64 factor = (pos1.Time != pos0.Time) ? (time - pos0.Time) / (pos1.Time - pos0.Time) : 0.0f;
		glm::vec3 position = glm::mix(pos0.Value, pos1.Value, glm::vec3(factor));
		return position;
	}

	glm::vec3 AnimationSystem::SampleScale(Animation::Channel& channel, float64 time, bool isLooping)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numScales = isLooping ? channel.Scales.GetSize() - 1 : channel.Scales.GetSize();

		Animation::Channel::KeyFrame scale0 = channel.Scales[0];
		Animation::Channel::KeyFrame scale1 = channel.Scales[0];
		if (numScales > 1)
		{
			for (uint32 i = 0; i < (numScales - 1); i++)
			{
				if (time < channel.Scales[i + 1].Time)
				{
					scale0 = channel.Scales[i];
					scale1 = channel.Scales[i + 1];
					break;
				}
			}
		}

		const float64 factor = (scale1.Time != scale0.Time) ? (time - scale0.Time) / (scale1.Time - scale0.Time) : 0.0f;
		glm::vec3 scale = glm::mix(scale0.Value, scale1.Value, glm::vec3(factor));
		return scale;
	}

	glm::quat AnimationSystem::SampleRotation(Animation::Channel& channel, float64 time, bool isLooping)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numRotations = isLooping ? channel.Rotations.GetSize() - 1 : channel.Rotations.GetSize();

		Animation::Channel::RotationKeyFrame rot0 = channel.Rotations[0];
		Animation::Channel::RotationKeyFrame rot1 = channel.Rotations[0];
		if (numRotations > 1)
		{
			for (uint32 i = 0; i < (numRotations - 1); i++)
			{
				if (time < channel.Rotations[i + 1].Time)
				{
					rot0 = channel.Rotations[i];
					rot1 = channel.Rotations[i + 1];
					break;
				}
			}
		}

		const float64 factor = (rot1.Time != rot0.Time) ? (time - rot0.Time) / (rot1.Time - rot0.Time) : 0.0;
		glm::quat rotation = glm::slerp(rot0.Value, rot1.Value, float32(factor));
		rotation = glm::normalize(rotation);
		return rotation;
	}

	void AnimationSystem::OnEntityAdded(Entity entity)
	{
		if (m_HasInitClock)
		{
			ECSCore* pECSCore = ECSCore::GetInstance();
			AnimationComponent& animationComp = pECSCore->GetComponent<AnimationComponent>(entity);
			animationComp.StartTime = GetTotalTimeInSeconds();
		}
	}

	bool AnimationSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.Phase = 0;
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				{
					{ RW, AnimationComponent::Type() }
				},
				&m_AnimationEntities,
				std::bind(&AnimationSystem::OnEntityAdded, this, std::placeholders::_1)
			},
		};

		RegisterSystem(systemReg);
		return true;
	}

	void AnimationSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		ComponentArray<AnimationComponent>* pAnimationComponents = pECSCore->GetComponentArray<AnimationComponent>();

		// This will only be run once a better way to handle this would be nice
		if (!m_HasInitClock)
		{
			m_Clock.Reset();

			// If we are initializing the clock we now need to init all the animations since they were not initialized in OnEntityAdded
			for (Entity entity : m_AnimationEntities.GetIDs())
			{
				AnimationComponent& animation = pAnimationComponents->GetData(entity);
				if (animation.StartTime != 0.0)
				{
					animation.StartTime = GetTotalTimeInSeconds();
				}
			}
			
			m_HasInitClock = true;
		}

		// Animation system has its own clock to keep track of time
		m_Clock.Tick();

		Timestamp deltatime = m_Clock.GetDeltaTime();
		for (Entity entity : m_AnimationEntities.GetIDs())
		{
			AnimationComponent&	animation = pAnimationComponents->GetData(entity);
			if (!animation.IsPaused)
			{
				Animate(animation);
			}
		}
	}
	
	AnimationSystem& AnimationSystem::GetInstance()
	{
		static AnimationSystem instance;
		return instance;
	}
}