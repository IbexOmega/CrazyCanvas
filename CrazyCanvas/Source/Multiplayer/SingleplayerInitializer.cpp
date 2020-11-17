#include "Multiplayer/SingleplayerInitializer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

#include "World/LevelObjectCreator.h"

namespace LambdaEngine
{
	void SingleplayerInitializer::Init()
	{
		using namespace LambdaEngine;

		MultiplayerUtils::SetIsSingleplayer(true);
	}

	void SingleplayerInitializer::Release()
	{
		MultiplayerUtils::SetIsSingleplayer(false);
	}

	void SingleplayerInitializer::Setup()
	{
		if (MultiplayerUtils::IsSingleplayer())
		{
			ClientSystem& clientSystem = ClientSystem::GetInstance();
			ClientBase* pClient = clientSystem.GetClient();

			PacketCreateLevelObject packet;
			packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
			packet.Position = glm::vec3(0.0f, 2.0f, 0.0f);
			packet.Forward = glm::vec3(1.0f, 0.0f, 0.0f);
			packet.Player.ClientUID = pClient->GetUID();
			packet.Player.WeaponNetworkUID = 1;
			packet.NetworkUID = 0;

			NetworkSegment* pNetworkSegment = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);

			pNetworkSegment->Write(&packet);

			clientSystem.OnPacketReceived(pClient, pNetworkSegment);

			pClient->ReturnPacket(pNetworkSegment);
		}
	}
}
