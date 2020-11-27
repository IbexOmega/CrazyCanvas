#include "ECS/Systems/Match/ShowerSystemBase.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/ShowerComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Multiplayer/Packet/PacketResetPlayerTexture.h"

ShowerSystemBase::~ShowerSystemBase()
{
	if (s_pInstance == this)
	{
		s_pInstance = nullptr;
	}
}

bool ShowerSystemBase::Init()
{
	using namespace LambdaEngine;

	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_PlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerBaseComponent::Type() },
					{ RW, PacketComponent<PacketResetPlayerTexture>::Type() },
				}
			}
		};
		systemReg.Phase = 1u;

		RegisterSystem(TYPE_NAME(ShowerSystemBase), systemReg);
	}

	s_pInstance = this;
	return true;
}

void ShowerSystemBase::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	FixedTickMainThreadInternal(deltaTime);
}

void ShowerSystemBase::Tick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	TickInternal(deltaTime);
}