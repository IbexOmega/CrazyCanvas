#include "Multiplayer/SingleplayerInitializer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Multiplayer/Packet/CreateLevelObject.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Application/API/Events/EventQueue.h"

#include "World/LevelObjectCreator.h"

namespace LambdaEngine
{
	void SingleplayerInitializer::InitSingleplayer()
	{
		using namespace LambdaEngine;

		CreateLevelObject packet;
		packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position = glm::vec3(0.0f, 2.0f, 0.0f);
		packet.Forward = glm::vec3(1.0f, 0.0f, 0.0f);
		packet.Player.TeamIndex = 0;
		packet.Player.IsMySelf = true;
		packet.NetworkUID = 0;

		MultiplayerUtils::SetIsSingleplayer(true);
		ClientSystem& clientSystem = ClientSystem::GetInstance();
		ClientBase* pClient = clientSystem.GetClient();

		NetworkSegment* pNetworkSegment = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);

		pNetworkSegment->Write(&packet);

		clientSystem.OnPacketReceived(pClient, pNetworkSegment);

		pClient->ReturnPacket(pNetworkSegment);
	}
}
