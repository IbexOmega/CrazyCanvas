#pragma once

#include "LambdaEngine.h"

#include "Containers/THashTable.h"

#include "ECS/ComponentType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

typedef LambdaEngine::THashTable<uint16, const LambdaEngine::ComponentType*> PacketTypeMap;

class PacketType
{
	friend class MultiplayerBase;
public:
	DECL_STATIC_CLASS(PacketType);

	static uint16 CREATE_LEVEL_OBJECT;
	static uint16 PLAYER_ACTION;
	static uint16 PLAYER_ACTION_RESPONSE;
	static uint16 WEAPON_FIRE;
	static uint16 FLAG_EDITED;

public:
	static const LambdaEngine::ComponentType* GetComponentType(uint16 packetType);
	static const PacketTypeMap& GetPacketTypeMap();

private:
	static void Init();

	static uint16 RegisterPacketType();

	template<typename Type>
	static uint16 RegisterPacketTypeWithComponent();

private:
	static uint16 s_PacketTypeCount;
	static PacketTypeMap s_PacketTypeToComponentType;
};

template<typename Type>
uint16 PacketType::RegisterPacketTypeWithComponent()
{
	const LambdaEngine::ComponentType* type = PacketComponent<Type>::Type();
	uint16 packetType = RegisterPacketType();
	PacketComponent<Type>::s_PacketType = packetType;
	s_PacketTypeToComponentType[packetType] = type;
	return packetType;
}