#include "World/SessionSettings.h"

#include "Game/GameConsole.h"
#include "Lobby/PlayerManagerClient.h"
#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/ClientHelper.h"
#include "Application/API/Events/EventQueue.h"

void SessionSettings::Init()
{
	using namespace LambdaEngine;

	// Reset Settings
	ConsoleCommand cmdResetSettings;
	cmdResetSettings.Init("session_settings_reset", false);
	cmdResetSettings.AddDescription("Set server ground acceleration.\n\t'session_settings_reset'");
	GameConsole::Get().BindCommand(cmdResetSettings, [](GameConsole::CallbackInput& input)->void
	{
		if(PlayerManagerClient::GetPlayerLocal()->IsHost())
			SessionSettings::SendPacketSessionSettings(ESessionSetting::RESET, true);
	});

	// Ground Acceleration
	ConsoleCommand cmdGroundAccel;
	cmdGroundAccel.Init("session_ground_acceleration", false);
	cmdGroundAccel.AddArg(Arg::EType::FLOAT);
	cmdGroundAccel.AddDescription("Set server ground acceleration.\n\t'session_ground_acceleration 100.0'");
	GameConsole::Get().BindCommand(cmdGroundAccel, [](GameConsole::CallbackInput& input)->void
	{
		if(PlayerManagerClient::GetPlayerLocal()->IsHost())
			SessionSettings::SendPacketSessionSettings(ESessionSetting::GROUND_ACCELERATION, input.Arguments.GetFront().Value.Float32);
	});

	// Air Acceleration
	ConsoleCommand cmdAirAccel;
	cmdAirAccel.Init("session_air_acceleration", false);
	cmdAirAccel.AddArg(Arg::EType::FLOAT);
	cmdAirAccel.AddDescription("Set server air acceleration.\n\t'session_air_acceleration 4.0'");
	GameConsole::Get().BindCommand(cmdAirAccel, [](GameConsole::CallbackInput& input)->void
	{
		if(PlayerManagerClient::GetPlayerLocal()->IsHost())
			SessionSettings::SendPacketSessionSettings(ESessionSetting::AIR_ACCELERATION, input.Arguments.GetFront().Value.Float32);
	});

	// Ground Friction
	ConsoleCommand cmdGroundFriction;
	cmdGroundFriction.Init("session_ground_friction", false);
	cmdGroundFriction.AddArg(Arg::EType::FLOAT);
	cmdGroundFriction.AddDescription("Set server ground friction.\n\t'session_ground_friction 15.0'");
	GameConsole::Get().BindCommand(cmdGroundFriction, [](GameConsole::CallbackInput& input)->void
	{
		if(PlayerManagerClient::GetPlayerLocal()->IsHost())
			SessionSettings::SendPacketSessionSettings(ESessionSetting::GROUND_FRICTION, input.Arguments.GetFront().Value.Float32);
	});

	LambdaEngine::EventQueue::RegisterEventHandler(SessionSettings::OnPacketSessionSettingsReceived);
}

void SessionSettings::Release()
{
	LambdaEngine::EventQueue::UnregisterEventHandler(SessionSettings::OnPacketSessionSettingsReceived);
}

bool SessionSettings::OnPacketSessionSettingsReceived(const PacketReceivedEvent<PacketSessionSettingChanged>& packetEvent)
{
	const PacketSessionSettingChanged& packet = packetEvent.Packet;

	if (PlayerManagerClient::GetPlayer(packetEvent.pClient)->IsHost())
	{
		// Notify all clients
		if (LambdaEngine::MultiplayerUtils::IsServer())
		{
			ServerHelper::SendBroadcast(packet, nullptr, packetEvent.pClient);

			SetSetting(packet.Setting, packet.Value);
		}
		else
		{
			SetSetting(packet.Setting, packet.Value);
		}
	}

	return false;
}

void SessionSettings::SendPacketSessionSettings(ESessionSetting setting, SettingValue value)
{
	SetSetting(setting, value);

	PacketSessionSettingChanged packet;
	packet.Setting = setting;
	packet.Value = value;

	ClientHelper::Send(packet);
}