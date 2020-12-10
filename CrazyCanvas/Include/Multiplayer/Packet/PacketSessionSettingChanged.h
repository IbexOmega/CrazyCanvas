#pragma once

#include "Multiplayer/Packet/Packet.h"
#include "World/SessionSettingsTypes.h"

#pragma pack(push, 1)
struct PacketSessionSettingChanged
{
	DECL_PACKET(PacketSessionSettingChanged);

	ESessionSetting Setting;
	SettingValue Value;
};
#pragma pack(pop)