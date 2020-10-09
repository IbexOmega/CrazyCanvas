#include "Game/ECS/Systems/Rendering/AnimationSystem.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

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

		// Call the graphs tick
		animation.Graph.Tick(GetDeltaTimeInSeconds(), GetTotalTimeInSeconds(), skeleton);

		if (m_ChangeState)
		{
			if (animation.Graph.GetCurrentState().GetName() == "running")
			{
				animation.Graph.TransitionToState("walking");
			}
			else
			{
				animation.Graph.TransitionToState("running");
			}
		}

		// Create localtransforms
		const TArray<SQT>& currentFrame = animation.Graph.GetCurrentFrame();
		for (uint32 i = 0; i < currentFrame.GetSize(); i++)
		{
			const SQT& sqt = currentFrame[i];

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

	bool AnimationSystem::OnKeyPressed(const KeyPressedEvent& keyPressedEvent)
	{
		if (keyPressedEvent.Key == EKey::KEY_Q)
		{
			m_ChangeState = true;
		}

		return false;
	}

	bool AnimationSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_AnimationEntities,
				.ComponentAccesses =
				{
					{ RW, AnimationComponent::Type() }
				},
				&m_AnimationEntities
			},
		};
		systemReg.Phase = 0;
		RegisterSystem(systemReg);

		EventQueue::RegisterEventHandler(this, &AnimationSystem::OnKeyPressed);
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

		m_ChangeState = false;
	}

	AnimationSystem& AnimationSystem::GetInstance()
	{
		static AnimationSystem instance;
		return instance;
	}
}
