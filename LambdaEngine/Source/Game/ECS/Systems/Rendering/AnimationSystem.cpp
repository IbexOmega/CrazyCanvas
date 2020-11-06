#include "Game/ECS/Systems/Rendering/AnimationSystem.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "Threading/API/ThreadPool.h"

namespace LambdaEngine
{
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
			},
			{
				.pSubscriber = &m_AttachedAnimationEntities,
				.ComponentAccesses =
				{
					{ RW, AnimationAttachedComponent::Type() },
					{ R, ParentComponent::Type() }
				},
			},
		};

		systemReg.Phase = 0;
		RegisterSystem(TYPE_NAME(AnimationSystem), systemReg);
		SetComponentOwner<AnimationComponent>({ .Destructor = &AnimationSystem::OnAnimationComponentDelete });
		return true;
	}

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
		VALIDATE(animation.pGraph != nullptr);
		animation.pGraph->Tick(skeleton, GetDeltaTimeInSeconds());

		// Create localtransforms
		const TArray<SQT>& currentFrame = animation.pGraph->GetCurrentFrame();
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

	void AnimationSystem::OnAnimationComponentDelete(AnimationComponent& animation, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);
		SAFEDELETE(animation.pGraph);
	}

	void AnimationSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

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

		for (Entity entity : m_AnimationEntities.GetIDs())
		{
			AnimationComponent& animation = pAnimationComponents->GetData(entity);
			if (!animation.IsPaused)
			{
				std::function<void()> func = [this, &animation]
				{
					Animate(animation);
				};

				m_JobIndices.EmplaceBack(ThreadPool::Execute(func));
			}
		}

		// Wait for all jobs to finish
		for (uint32 index : m_JobIndices)
		{
			ThreadPool::Join(index);
		}
		m_JobIndices.Clear();

		//Update Attached Animation Components
		const ComponentArray<ParentComponent>* pParentComponents = pECSCore->GetComponentArray<ParentComponent>();
		ComponentArray<AnimationAttachedComponent>* pAnimationAttachedComponents = pECSCore->GetComponentArray<AnimationAttachedComponent>();

		for (Entity entity : m_AttachedAnimationEntities)
		{
			const ParentComponent& parentComponent = pParentComponents->GetConstData(entity);

			if (parentComponent.Attached)
			{
				AnimationAttachedComponent& animationAttachedComponent = pAnimationAttachedComponents->GetData(entity);
				AnimationComponent& parentAnimationComponent = pAnimationComponents->GetData(parentComponent.Parent);

				if (auto jointIndexIt = parentAnimationComponent.Pose.pSkeleton->JointMap.find(animationAttachedComponent.JointName);
					jointIndexIt != parentAnimationComponent.Pose.pSkeleton->JointMap.end())
				{
					animationAttachedComponent.Transform = parentAnimationComponent.Pose.GlobalTransforms[jointIndexIt->second];
				}
				else
				{
					LOG_ERROR("[AnimationSystem]: Joint %s could not be found for Attached Animation Component", animationAttachedComponent.JointName.c_str());
				}
			}
		}
	}

	AnimationSystem& AnimationSystem::GetInstance()
	{
		static AnimationSystem instance;
		return instance;
	}
}
