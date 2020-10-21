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
					{NDA, FlagComponent::Type()},
					{RW, PositionComponent::Type()},
					{RW, RotationComponent::Type()},
					{R, OffsetComponent::Type()},
					{R, ParentComponent::Type()},
				}
			}
		};
		systemReg.SubscriberRegistration.AdditionalAccesses =
		{
			{R, TeamComponent::Type()}
		};
		systemReg.Phase = 1;

		RegisterSystem(systemReg);
	}

	s_Instance = this;
	return true;
}

void FlagSystemBase::Tick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	if (m_Flags.Size() > 1)
	{
		LOG_WARNING("[FlagSystemBase]: More than one flag entity exists");
	}

	ECSCore* pECS = ECSCore::GetInstance();

	Entity flagEntity = m_Flags[0];

	const ParentComponent& parentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);

	if (parentComponent.Attached)
	{
		const PositionComponent& parentPositionComponent = pECS->GetComponent<PositionComponent>(parentComponent.Parent);
		const RotationComponent& parentRotationComponent = pECS->GetComponent<RotationComponent>(parentComponent.Parent);

		const OffsetComponent& flagOffsetComponent	= pECS->GetConstComponent<OffsetComponent>(flagEntity);
		PositionComponent& flagPositionComponent	= pECS->GetComponent<PositionComponent>(flagEntity);
		RotationComponent& flagRotationComponent	= pECS->GetComponent<RotationComponent>(flagEntity);

		flagPositionComponent.Position		= parentPositionComponent.Position;
		flagRotationComponent.Quaternion	= flagRotationComponent.Quaternion;
	}

	TickInternal(deltaTime);
}
