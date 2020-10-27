#include "Game/ECS/Systems/Networking/NetworkSystem.h"

#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

namespace LambdaEngine
{
	NetworkSystem NetworkSystem::s_Instance;

	NetworkSystem::NetworkSystem()
	{

	}

	NetworkSystem::~NetworkSystem()
	{

	}

	bool NetworkSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_Entities,
				.ComponentAccesses =
				{
					{RW, NetworkComponent::Type()}
				},
				.OnEntityAdded = std::bind_front(&NetworkSystem::OnEntityAdded, this),
				.OnEntityRemoval = std::bind_front(&NetworkSystem::OnEntityRemoved, this)
			}
		};

		systemReg.Phase = 0;

		RegisterSystem(TYPE_NAME(NetworkSystem), systemReg);
		return true;
	}

	void NetworkSystem::OnEntityAdded(Entity entity)
	{
		NetworkComponent pNetworkComponent = ECSCore::GetInstance()->GetComponent<NetworkComponent>(entity);
		MultiplayerUtils::RegisterEntity(entity, pNetworkComponent.NetworkUID);
	}

	void NetworkSystem::OnEntityRemoved(Entity entity)
	{
		MultiplayerUtils::UnregisterEntity(entity);
	}
}