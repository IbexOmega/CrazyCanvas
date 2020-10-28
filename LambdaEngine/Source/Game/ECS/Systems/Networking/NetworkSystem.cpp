#include "Game/ECS/Systems/Networking/NetworkSystem.h"

#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

namespace LambdaEngine
{
	NetworkSystem NetworkSystem::s_Instance;

	bool NetworkSystem::Init()
	{
		const ComponentOwnership<NetworkComponent> networkComponentOwnership =
		{
			.Constructor = std::bind_front(&NetworkSystem::NetworkComponentConstructor),
			.Destructor = std::bind_front(&NetworkSystem::NetworkComponentDestructor)
		};
		SetComponentOwner<NetworkComponent>(networkComponentOwnership);

		return true;
	}

	void NetworkSystem::NetworkComponentConstructor(NetworkComponent& networkComponent, Entity entity)
	{
		MultiplayerUtils::RegisterEntity(entity, networkComponent.NetworkUID);
	}

	void NetworkSystem::NetworkComponentDestructor(NetworkComponent& networkComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(networkComponent);
		MultiplayerUtils::UnregisterEntity(entity);
	}
}
