#include "ECS/Systems/Player/WeaponSystemServer.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

/*
* WeaponSystemServer
*/

void WeaponSystemServer::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PacketComponent<PacketPlayerAction>>*			pPlayerActionPackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerAction>>();
	ComponentArray<PacketComponent<PacketPlayerActionResponse>>*	pPlayerResponsePackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerActionResponse>>();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();

	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComp = pWeaponComponents->GetData(weaponEntity);
		Entity remotePlayerEntity	= weaponComp.WeaponOwner;

		// Update weapon
		const bool hasAmmo		= weaponComp.CurrentAmmunition > 0;
		const bool isReloading	= weaponComp.ReloadClock > 0.0f;
		const bool onCooldown	= weaponComp.CurrentCooldown > 0.0f;

		// Update reload and cooldown timers
		UpdateWeapon(weaponComp, dt);

		PacketComponent<PacketPlayerAction>&			actionsRecived	= pPlayerActionPackets->GetData(remotePlayerEntity);
		PacketComponent<PacketPlayerActionResponse>&	responsesToSend	= pPlayerResponsePackets->GetData(remotePlayerEntity);
		TQueue<PacketPlayerActionResponse>&	packetsToSend	= responsesToSend.GetPacketsToSend();
		const TArray<PacketPlayerAction>&	packetsRecived	= actionsRecived.GetPacketsReceived();

		// Handle packets
		const uint32 packetCount = packetsRecived.GetSize();
		for (uint32 i = 0; i < packetCount; i++)
		{
			// Start reload
			if (packetsRecived[i].StartedReload && !isReloading)
			{
				weaponComp.ReloadClock = weaponComp.ReloadTime;
			}
			else if (packetsRecived[i].FiredAmmo != EAmmoType::AMMO_TYPE_NONE && !onCooldown)
			{
				// Only send if the weapon has ammo on the server
				if (hasAmmo)
				{
					// Update position and orientation of weapon component
					const EAmmoType ammoType = packetsRecived[i].FiredAmmo;
					packetsToSend.back().FiredAmmo = ammoType;

					// Handle fire
					weaponComp.CurrentCooldown = 1.0f / weaponComp.FireRate;

					// Create projectile
					Fire(ammoType, weaponEntity);
				}
			}
		}
	}
}

bool WeaponSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	if (!WeaponSystem::InitInternal())
	{
		return false;
	}

	// Register system
	{
		PlayerGroup playerGroup = {};
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= R;
		playerGroup.Rotation.Permissions	= R;
		playerGroup.Velocity.Permissions	= R;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_RemotePlayerEntities,
				.ComponentAccesses =
				{
					{ NDA,	PlayerBaseComponent::Type() },
					{ R,	PacketComponent<PacketPlayerAction>::Type() },
					{ RW,	PacketComponent<PacketPlayerActionResponse>::Type() },
				},
				.ComponentGroups = { &playerGroup }
			}
		};

		systemReg.SubscriberRegistration.AdditionalAccesses = GetFireProjectileComponentAccesses();
		systemReg.Phase = 1;

		RegisterSystem(TYPE_NAME(WeaponSystem), systemReg);
	}

	return true;
}