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
		// Make sure we have enough matrices
		Skeleton& skeleton = *animation.Pose.pSkeleton;
		if (animation.Pose.LocalTransforms.GetSize() < skeleton.Joints.GetSize())
		{
			animation.Pose.LocalTransforms.Resize(skeleton.Joints.GetSize(), glm::mat4(1.0f));
		}

		if (animation.Pose.GlobalTransforms.GetSize() < skeleton.Joints.GetSize())
		{
			animation.Pose.GlobalTransforms.Resize(skeleton.Joints.GetSize(), glm::mat4(1.0f));
		}

		AnimationBlendState& blendState = animation.State.GetCurrentBlendState();
		blendState.CalculateWeights(); // Calculates the weights if they are dirty

		TArray<TArray<SQT>> sqtArrays;
		sqtArrays.Reserve(blendState.GetBlendInfoCount());

		for (const AnimationBlendInfo& blendInfo : blendState)
		{
			VALIDATE(animation.State.HasClip(blendInfo.AnimationName) == true);

			ClipState& clip = animation.State.GetClip(blendInfo.AnimationName);
			Animation& anim = *ResourceManager::GetAnimation(clip.AnimationGUID);
			
			// Get localtime for the animation-clip
			float64 localTime = (GetTotalTimeInSeconds() - clip.StartTime) * fabs(clip.PlaybackSpeed);
			if (clip.IsLooping)
			{
				if (clip.NumLoops != INFINITE_LOOPS)
				{
					float64 totalDuration = clip.NumLoops * anim.DurationInSeconds();
					localTime = glm::clamp(localTime, 0.0, totalDuration);
				}

				localTime = fmod(localTime, anim.DurationInSeconds());
			}
			else
			{
				localTime = glm::clamp(localTime, 0.0, anim.DurationInSeconds());
			}

			clip.LocalTime = localTime / anim.DurationInSeconds();
			if (clip.PlaybackSpeed < 0.0)
			{
				clip.LocalTime = 1.0 - clip.LocalTime;
			}

			// Interpolate between keyframes
			sqtArrays.EmplaceBack(CalculateSQT(anim, skeleton, clip.LocalTime, clip.IsLooping));
		}

		// Blend
		if (sqtArrays.GetSize() > 1)
		{
			// Get blend weight factor


			// Perform blending
			const float32 weight = blendState.GetBlendInfo(i).NormalizedWeight;
			for (uint32 i = 0; i < sqtArrays.GetSize() - 1; i++)
			{
				TArray<SQT>& array0 = sqtArrays[i];
				TArray<SQT>& array1 = sqtArrays[i + 1];

				// TODO: Other blending technique than a simple learp is necessary if we want to blend more than two meshes
				for (uint32 j = 0; j < array0.GetSize(); j++)
				{
					SQT& sqt0 = array0[j];
					SQT& sqt1 = array1[j];

					// Save in array slot 0
					sqt0.Translation	= glm::mix(sqt1.Translation, sqt0.Translation, glm::vec3(weight));
					sqt0.Scale			= glm::mix(sqt1.Scale, sqt0.Scale, glm::vec3(weight));
					sqt0.Rotation		= glm::slerp(sqt1.Rotation, sqt0.Rotation, weight);
					sqt0.Rotation		= glm::normalize(sqt0.Rotation);
				}
			}
		}

		// Create localtransforms
		for (uint32 i = 0; i < sqtArrays[0].GetSize(); i++)
		{
			SQT& sqt = sqtArrays[0][i];

			glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), sqt.Translation);
			transform			= transform * glm::toMat4(sqt.Rotation);
			transform			= glm::scale(transform, sqt.Scale);
			animation.Pose.LocalTransforms[i] = transform;
		}
		
		// Create global transforms
		for (uint32 i = 0; i < skeleton.Joints.GetSize(); i++)
		{
			const Joint& joint = skeleton.Joints[i];
			animation.Pose.GlobalTransforms[i] = skeleton.InverseGlobalTransform * ApplyParent(joint, skeleton, animation.Pose.LocalTransforms) * joint.InvBindTransform;
		}
	}

	TArray<SQT> AnimationSystem::CalculateSQT(Animation& animation, Skeleton& skeleton, float64 normalizedTime, bool isLooping)
	{
		const float64 time = normalizedTime * animation.DurationInTicks;

		TArray<SQT> sqt;
		sqt.Resize(skeleton.Joints.GetSize());

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
			
			const uint32 boneID = it->second;
			sqt[boneID] = SQT(position, scale, rotation);
		}

		return sqt;
	}

	glm::mat4 AnimationSystem::ApplyParent(const Joint& joint, Skeleton& skeleton, TArray<glm::mat4>& matrices)
	{
		int32 parentID	= joint.ParentBoneIndex;
		int32 myID		= skeleton.JointMap[joint.Name];
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

				// TODO: Fix better way for this
				const AnimationBlendState& blendState = animation.State.GetCurrentBlendState();
				for (const AnimationBlendInfo& blendInfo : blendState)
				{
					VALIDATE(animation.State.HasClip(blendInfo.AnimationName) == true);

					ClipState& clip = animation.State.GetClip(blendInfo.AnimationName);
					clip.StartTime = GetTotalTimeInSeconds();
				}
			}
			
			m_HasInitClock = true;
		}

		// Animation system has its own clock to keep track of time
		m_Clock.Tick();

		Timestamp deltatime = m_Clock.GetDeltaTime();
		for (Entity entity : m_AnimationEntities.GetIDs())
		{
			AnimationComponent& animation = pAnimationComponents->GetData(entity);
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
