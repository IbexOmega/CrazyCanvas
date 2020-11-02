#pragma once

#include "LambdaEngine.h"

#include "Containers/THashTable.h"

#include "ECS/ComponentType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"

typedef LambdaEngine::THashTable<uint16, IPacketReceivedEvent*> PacketTypeMap;

class PacketType
{
	friend class MultiplayerBase;
public:
	DECL_STATIC_CLASS(PacketType);

	static uint16 CREATE_LEVEL_OBJECT;
	static uint16 DELETE_LEVEL_OBJECT;
	static uint16 PLAYER_ACTION;
	static uint16 PLAYER_ACTION_RESPONSE;
	static uint16 WEAPON_FIRE;
	static uint16 HEALTH_CHANGED;
	static uint16 FLAG_EDITED;
	static uint16 TEAM_SCORED;
	static uint16 GAME_OVER;
	static uint16 CONFIGURE_SERVER;

public:
	static IPacketReceivedEvent* GetPacketReceivedEventPointer(uint16 packetType);
	static const PacketTypeMap& GetPacketTypeMap();

private:
	static void Init();

	template<typename Type>
	static uint16 RegisterPacketType(const LambdaEngine::ComponentType* pType = nullptr);

	template<typename Type>
	static uint16 RegisterPacketTypeWithComponent();

	static uint16 RegisterPacketTypeRaw();

	static void Release();

private:
	static uint16 s_PacketTypeCount;
	static PacketTypeMap s_PacketTypeToEvent;
};

template<typename Type>
uint16 PacketType::RegisterPacketType(const LambdaEngine::ComponentType* pType)
{
	PacketReceivedEvent<Type>* pEvent = DBG_NEW PacketReceivedEvent<Type>(pType);
	uint16 packetType = RegisterPacketTypeRaw();
	VALIDATE_MSG(Type::s_Type == 0, "PacketType already Registered!");
	Type::s_Type = packetType;
	s_PacketTypeToEvent[packetType] = pEvent;

	return packetType;
}

template<typename Type>
uint16 PacketType::RegisterPacketTypeWithComponent()
{
	uint16 packetType = RegisterPacketType<Type>(PacketComponent<Type>::Type());
	PacketComponent<Type>::s_PacketType = packetType;
	return packetType;
}