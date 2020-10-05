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

		Mesh* pMesh = ResourceManager::GetMesh(mesh.MeshGUID);
		VALIDATE(pMesh);

		Skeleton* pSkeleton = pMesh->pSkeleton;
		VALIDATE(pSkeleton);

		// Move timer
		const float64 ticksPerSeconds	= animation.PlaybackSpeed * pAnimation->TicksPerSecond;
		const float64 deltaTicks		= ticksPerSeconds * deltaTime.AsSeconds();
		animation.DurationInTicks += deltaTicks;
		if (animation.DurationInTicks > pAnimation->DurationInTicks)
		{
			animation.DurationInTicks = 0.0f;
		}

		// Make sure we have enough matrices
		animation.BoneMatrices.Resize(pSkeleton->Bones.GetSize());
		
		// Find keyframes
		for (Animation::Channel& channel : pAnimation->Channels)
		{
			// Retrive the bone ID
			auto it = pSkeleton->BoneMap.find(channel.Name);
			if (it == pSkeleton->BoneMap.end())
			{
				continue;
			}

			const uint32 boneID = it->second;

			// Interpolate position
			glm::vec3 position;
			{
				Animation::Channel::KeyFrame pos0 = channel.Positions[0];
				Animation::Channel::KeyFrame pos1 = channel.Positions[0];
				if (channel.Positions.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Positions.GetSize() - 1); i++)
					{
						if (animation.DurationInTicks < channel.Positions[i + 1].Time)
						{
							pos0 = channel.Positions[i];
							pos1 = channel.Positions[i + 1];
							break;
						}
					}
				}

				const float32 factor = (pos1.Time != pos0.Time) ? (animation.DurationInTicks - pos0.Time) / (pos1.Time - pos0.Time) : 0.0f;
				position = glm::mix(pos0.Value, pos1.Value, glm::vec3(factor));
			}

			// Interpolate rotation
			glm::quat rotation;
			{
				Animation::Channel::RotationKeyFrame rot0 = channel.Rotations[0];
				Animation::Channel::RotationKeyFrame rot1 = channel.Rotations[0];
				if (channel.Rotations.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Rotations.GetSize() - 1); i++)
					{
						if (animation.DurationInTicks < channel.Rotations[i + 1].Time)
						{
							rot0 = channel.Rotations[i];
							rot1 = channel.Rotations[i + 1];
							break;
						}
					}
				}

				const float32 factor = (rot1.Time != rot0.Time) ? (animation.DurationInTicks - rot0.Time) / (rot1.Time - rot0.Time) : 0.0f;
				rotation = glm::slerp(rot0.Value, rot1.Value, factor);
				rotation = glm::normalize(rotation);
			}

			// Interpolate scale
			glm::vec3 scale;
			{
				Animation::Channel::KeyFrame scale0 = channel.Scales[0];
				Animation::Channel::KeyFrame scale1 = channel.Scales[0];
				if (channel.Positions.GetSize() > 1)
				{
					for (uint32 i = 0; i < (channel.Scales.GetSize() - 1); i++)
					{
						if (animation.DurationInTicks < channel.Scales[i + 1].Time)
						{
							scale0 = channel.Scales[i];
							scale1 = channel.Scales[i + 1];
							break;
						}
					}
				}

				const float32 factor = (scale1.Time != scale0.Time) ? (animation.DurationInTicks - scale0.Time) / (scale1.Time - scale0.Time) : 0.0f;
				scale = glm::mix(scale0.Value, scale1.Value, glm::vec3(factor));
			}

			// Calculate transform
			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
			transform			= transform * glm::toMat4(rotation);
			transform			= glm::scale(transform, scale);

			// Increase boneID
			animation.BoneMatrices[boneID] = transform;
		}

		// Apply parent transform
		TArray<glm::mat4> tempBones = animation.BoneMatrices;
		for (uint32 i = 0; i < pSkeleton->Bones.GetSize(); i++)
		{
			Bone& bone = pSkeleton->Bones[i];
			tempBones[i] = pSkeleton->GlobalTransform * ApplyParent(bone, *pSkeleton, animation.BoneMatrices) * bone.OffsetTransform;
		}

		animation.BoneMatrices.Swap(tempBones);
	}

	glm::mat4 AnimationSystem::ApplyParent(Bone& bone, Skeleton& skeleton, TArray<glm::mat4>& matrices)
	{
		int32 parentID	= bone.ParentBoneIndex;
		int32 myID		= skeleton.BoneMap[bone.Name];
		if (parentID == -1)
		{
			return matrices[myID];
		}

		return ApplyParent(skeleton.Bones[parentID], skeleton, matrices) * matrices[myID];
	}

	bool AnimationSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.Phase = 0;
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				{
					{ RW,	AnimationComponent::Type() }, 
					{ R,	MeshComponent::Type() }
				},
				&m_AnimationEntities,	
				std::bind(&AnimationSystem::OnEntityAdded, this, std::placeholders::_1), 
				std::bind(&AnimationSystem::OnEntityRemoved, this, std::placeholders::_1) 
			},
		};

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
			if (!animation.IsPaused)
			{
				Animate(deltaTime, animation, mesh);
			}
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