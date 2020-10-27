#include "Multiplayer/SingleplayerInitializer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Multiplayer/Packet/PacketType.h"

#include "World/LevelObjectCreator.h"

namespace LambdaEngine
{
	void SingleplayerInitializer::InitSingleplayer()
	{
		using namespace LambdaEngine;

		MultiplayerUtils::SetIsSingleplayer(true);
		ClientSystem& clientSystem = ClientSystem::GetInstance();
		ClientBase* pClient = clientSystem.GetClient();

		NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CREATE_LEVEL_OBJECT);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt8(static_cast<uint8>(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER));
		encoder.WriteBool(true);
		encoder.WriteInt32(0);
		encoder.WriteVec3(glm::vec3(0.0f, 2.0f, 0.0f));
		encoder.WriteVec3(glm::vec3(1.0f, 0.0f, 0.0f));
		encoder.WriteUInt32(0);
		clientSystem.OnPacketReceived(pClient, pPacket);
		pClient->ReturnPacket(pPacket);
	}
}
