#include "ECS/Systems/Match/FlagSystemBase.h"

#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Team/TeamComponent.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "ECS/ECSCore.h"

#include "Log/Log.h"

FlagSystemBase::~FlagSystemBase()
{
	if (s_Instance == this)
	{
		s_Instance = nullptr;
	}
}

bool FlagSystemBase::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_Flags,
				.ComponentAccesses =
				{
					{ NDA, FlagComponent::Type() },
					{ RW, PositionComponent::Type() },
					{ RW, RotationComponent::Type() },
					{ R, OffsetComponent::Type() },
					{ R, ParentComponent::Type() },
				}
			}
		};
		systemReg.SubscriberRegistration.AdditionalAccesses =
		{
			{R, TeamComponent::Type()}
		};
		systemReg.Phase = 1u;

		InternalAddAdditionalRequiredFlagComponents(systemReg.SubscriberRegistration.EntitySubscriptionRegistrations[0].ComponentAccesses);
		InternalAddAdditionalAccesses(systemReg.SubscriberRegistration.AdditionalAccesses);

		RegisterSystem(TYPE_NAME(FlagSystemBase), systemReg);
	}

	s_Instance = this;
	return true;
}

void FlagSystemBase::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	if (m_Flags.Size() > 1)
	{
		LOG_WARNING("[FlagSystemBase]: More than one flag entity exists");
	}

	FixedTickMainThreadInternal(deltaTime);
}

void FlagSystemBase::Tick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	if (m_Flags.Size() > 1)
	{
		LOG_WARNING("[FlagSystemBase]: More than one flag entity exists");
	}

	TickInternal(deltaTime);
}

void FlagSystemBase::CalculateAttachedFlagPosition(
	glm::vec3& flagPosition, 
	glm::quat& flagRotation, 
	const glm::vec3& flagOffset, 
	const glm::vec3& parentPosition, 
	const glm::quat parentRotation)
{
	flagRotation = parentRotation;
	flagRotation.x = 0.0f;
	flagRotation.z = 0.0f;
	flagRotation = glm::normalize(flagRotation);
	flagPosition = parentPosition + flagOffset + flagRotation * glm::vec3(0.0f, 0.0f, 0.1f);
}
