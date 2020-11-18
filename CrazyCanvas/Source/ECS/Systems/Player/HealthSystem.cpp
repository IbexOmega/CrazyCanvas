#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/Systems/Player/HealthSystemClient.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Events/GameplayEvents.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include <mutex>

/*
* HealthSystem
*/

LambdaEngine::TUniquePtr<HealthSystem> HealthSystem::s_Instance = nullptr;

bool HealthSystem::Init()
{
	using namespace LambdaEngine;

	if (MultiplayerUtils::IsServer())
	{
		s_Instance = DBG_NEW HealthSystemServer();
	}
	else
	{
		s_Instance = DBG_NEW HealthSystemClient();
	}

	return s_Instance->InitInternal();
}

void HealthSystem::Release()
{
	s_Instance.Reset();
}

void HealthSystem::CreateBaseSystemRegistration(LambdaEngine::SystemRegistration& systemReg)
{
	using namespace LambdaEngine;

	// Fill in Base System Registration
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= NDA;
		playerGroup.Rotation.Permissions	= NDA;
		playerGroup.Velocity.Permissions	= NDA;

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_HealthEntities,
				.ComponentAccesses =
				{
					{ RW, HealthComponent::Type() },
					{ RW, PacketComponent<PacketHealthChanged>::Type() }
				}
			},
		};

		// After weaponsystem
		systemReg.Phase = LAST_PHASE;
	}
}
