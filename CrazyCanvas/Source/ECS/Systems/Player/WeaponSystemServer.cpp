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
		const bool isReloading	= weaponComp.ReloadClock > 0.0f;
		const bool onCooldown	= weaponComp.CurrentCooldown > 0.0f;

		// Update reload and cooldown timers
		UpdateWeapon(weaponComp, dt);

		PacketComponent<PacketPlayerAction>&			actionsRecived	= pPlayerActionPackets->GetData(remotePlayerEntity);
		PacketComponent<PacketPlayerActionResponse>&	responsesToSend	= pPlayerResponsePackets->GetData(remotePlayerEntity);
		TQueue<PacketPlayerActionResponse>&				packetsToSend	= responsesToSend.GetPacketsToSend();
		const TArray<PacketPlayerAction>&				packetsRecived	= actionsRecived.GetPacketsReceived();

		// Handle packets
		const uint32 packetCount = packetsRecived.GetSize();
		for (uint32 i = 0; i < packetCount; i++)
		{
			// Start reload
			const EAmmoType ammoType = packetsRecived[i].FiredAmmo;
			if (packetsRecived[i].StartedReload && !isReloading)
			{
				weaponComp.ReloadClock = weaponComp.ReloadTime;
			}
			else if (ammoType != EAmmoType::AMMO_TYPE_NONE && !onCooldown)
			{
				auto ammoState = weaponComp.WeaponTypeAmmo.find(ammoType);
				VALIDATE(ammoState != weaponComp.WeaponTypeAmmo.end())

				// Only send if the weapon has ammo on the server
				const bool hasAmmo = ammoState->second.first;
				if (hasAmmo)
				{
					//Calculate Weapon Fire Properties (Position, Velocity and Team)
					glm::vec3 firePosition;
					glm::vec3 fireVelocity;
					uint32 playerTeam;
					CalculateWeaponFireProperties(weaponEntity, firePosition, fireVelocity, playerTeam);

					// Update position and orientation of weapon component
					packetsToSend.back().FiredAmmo		= ammoType;
					packetsToSend.back().WeaponPosition	= firePosition;
					packetsToSend.back().WeaponVelocity	= fireVelocity;

					// Handle fire
					weaponComp.CurrentCooldown = 1.0f / weaponComp.FireRate;

					// Create projectile
					Fire(weaponEntity, weaponComp, ammoType, firePosition, fireVelocity, playerTeam);
				}
			}
		}
	}
}

bool WeaponSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup = {};
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= R;
		playerGroup.Rotation.Permissions	= R;
		playerGroup.Velocity.Permissions	= R;

		SystemRegistration systemReg = {};
		WeaponSystem::CreateBaseSystemRegistration(systemReg);

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
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
		);

		RegisterSystem(TYPE_NAME(WeaponSystem), systemReg);
	}

	return true;
}