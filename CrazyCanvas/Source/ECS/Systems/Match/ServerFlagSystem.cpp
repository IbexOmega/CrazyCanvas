#include "ECS/Systems/Match/ServerFlagSystem.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Team/TeamComponent.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Physics/CollisionGroups.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Match/Match.h"

#include "Multiplayer/Packet/PacketFlagEdited.h"

#include "Application/API/Events/EventQueue.h"
#include "Events/MatchEvents.h"

ServerFlagSystem::ServerFlagSystem()
{

}

ServerFlagSystem::~ServerFlagSystem()
{

}

void ServerFlagSystem::OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::RW,	FlagComponent::Type() },
		{ ComponentPermissions::R,	TeamComponent::Type() },
		{ ComponentPermissions::R,	CharacterColliderComponent::Type() },
		{ ComponentPermissions::RW,	DynamicCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	OffsetComponent::Type() },
		{ ComponentPermissions::RW,	PacketComponent<PacketFlagEdited>::Type() },
	};

	job.Function = [flagEntity, playerEntity]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		FlagComponent& flagComponent = pECS->GetComponent<FlagComponent>(flagEntity);

		if (EngineLoop::GetTimeSinceStart() > flagComponent.DroppedTimestamp + flagComponent.PickupCooldown)
		{
			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

			TeamComponent flagTeamComponent = {};

			bool validFlagPickup = false;
			if (pTeamComponents->GetConstIf(flagEntity, flagTeamComponent))
			{
				const TeamComponent& playerTeamComponent = pECS->GetConstComponent<TeamComponent>(playerEntity);
				validFlagPickup = (flagTeamComponent.TeamIndex != playerTeamComponent.TeamIndex);
			}
			else
			{
				//If the flag doesn't have a team component we don't care about what team collides with it
				validFlagPickup = true;
			}

			if (validFlagPickup)
			{
				const CharacterColliderComponent& playerCollisionComponent	= pECS->GetConstComponent<CharacterColliderComponent>(playerEntity);

				DynamicCollisionComponent& flagCollisionComponent		= pECS->GetComponent<DynamicCollisionComponent>(flagEntity);
				ParentComponent& flagParentComponent					= pECS->GetComponent<ParentComponent>(flagEntity);
				OffsetComponent& flagOffsetComponent					= pECS->GetComponent<OffsetComponent>(flagEntity);
				PacketComponent<PacketFlagEdited>& flagPacketComponent	= pECS->GetComponent<PacketComponent<PacketFlagEdited>>(flagEntity);

				//Set HasBeenPickedUp to true
				flagComponent.HasBeenPickedUp = true;

				//Disable the player-flag trigger shape
				TArray<PxShape*> flagShapes(flagCollisionComponent.pActor->getNbShapes());
				flagCollisionComponent.pActor->getShapes(flagShapes.GetData(), flagShapes.GetSize());
				for (PxShape* pFlagShape : flagShapes)
				{
					ShapeUserData* pShapeUserData = reinterpret_cast<ShapeUserData*>(pFlagShape->userData);
					EFlagColliderType flagColliderType = reinterpret_cast<EFlagColliderType*>(pShapeUserData->pUserData)[0];

					if (flagColliderType == EFlagColliderType::FLAG_COLLIDER_TYPE_PLAYER)
					{
						pFlagShape->acquireReference();
						flagCollisionComponent.pActor->detachShape(*pFlagShape);
						pFlagShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
						pFlagShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
						flagCollisionComponent.pActor->attachShape(*pFlagShape);
						pFlagShape->release();
					}
				}

				//Set Flag Carrier (Parent)
				flagParentComponent.Attached	= true;
				flagParentComponent.Parent		= playerEntity;

				//Set Flag Offset
				const physx::PxBounds3& playerBoundingBox = playerCollisionComponent.pController->getActor()->getWorldBounds();
				flagOffsetComponent.Offset = glm::vec3(0.0f, playerBoundingBox.getDimensions().y / 2.0f, 0.0f);

				//Send Packet
				PacketFlagEdited packet	= {};
				packet.FlagPacketType		= EFlagPacketType::FLAG_PACKET_TYPE_PICKED_UP;
				packet.PickedUpNetworkUID	= MultiplayerUtils::GetNetworkUID(playerEntity);
				flagPacketComponent.SendPacket(packet);
			}
		}
	};

	pECS->ScheduleJobASAP(job);
}

void ServerFlagSystem::OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::RW,	FlagComponent::Type() },
		{ ComponentPermissions::RW,	DynamicCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	PositionComponent::Type() },
		{ ComponentPermissions::RW,	PacketComponent<PacketFlagEdited>::Type() },
	};

	job.Function = [flagEntity, dropPosition]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		FlagComponent& flagComponent							= pECS->GetComponent<FlagComponent>(flagEntity);
		DynamicCollisionComponent& flagCollisionComponent		= pECS->GetComponent<DynamicCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent					= pECS->GetComponent<ParentComponent>(flagEntity);
		PositionComponent& flagPositionComponent				= pECS->GetComponent<PositionComponent>(flagEntity);
		PacketComponent<PacketFlagEdited>& flagPacketComponent	= pECS->GetComponent<PacketComponent<PacketFlagEdited>>(flagEntity);

		//Set Flag Spawn Timestamp
		flagComponent.DroppedTimestamp = EngineLoop::GetTimeSinceStart();

		//Enable the player-flag trigger shape
		TArray<PxShape*> flagShapes(flagCollisionComponent.pActor->getNbShapes());
		flagCollisionComponent.pActor->getShapes(flagShapes.GetData(), flagShapes.GetSize());
		for (PxShape* pFlagShape : flagShapes)
		{
			ShapeUserData* pShapeUserData		= reinterpret_cast<ShapeUserData*>(pFlagShape->userData);
			EFlagColliderType flagColliderType	= reinterpret_cast<EFlagColliderType*>(pShapeUserData->pUserData)[0];

			if (flagColliderType == EFlagColliderType::FLAG_COLLIDER_TYPE_PLAYER)
			{
				pFlagShape->acquireReference();
				flagCollisionComponent.pActor->detachShape(*pFlagShape);
				pFlagShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
				pFlagShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
				flagCollisionComponent.pActor->attachShape(*pFlagShape);
				pFlagShape->release();
			}
		}

		PxTransform transform;
		transform.p.x = dropPosition.x;
		transform.p.y = dropPosition.y;
		transform.p.z = dropPosition.z;

		glm::quat rotation = glm::quatLookAt(glm::vec3(1.0f, 0.0f, 0.0f), g_DefaultUp);
		transform.q.x = rotation.x;
		transform.q.y = rotation.y;
		transform.q.z = rotation.z;
		transform.q.w = rotation.w;

		flagCollisionComponent.pActor->setKinematicTarget(transform);

		//Set Flag Carrier (None)
		flagParentComponent.Attached	= false;
		flagParentComponent.Parent		= UINT32_MAX;

		// Set Position
		flagPositionComponent.Position	= dropPosition;

		// Send Packet
		PacketFlagEdited packet	= {};
		packet.FlagPacketType	= EFlagPacketType::FLAG_PACKET_TYPE_DROPPED;
		packet.DroppedPosition	= dropPosition;
		flagPacketComponent.SendPacket(packet);
	};

	pECS->ScheduleJobASAP(job);
}

void ServerFlagSystem::OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	OnFlagPickedUp(entity1, entity0);
}

void ServerFlagSystem::OnDeliveryPointFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::R,	TeamComponent::Type() },
		{ ComponentPermissions::R,	ParentComponent::Type() },
	};

	job.Function = [entity0, entity1]()
	{
		Entity flagEntity = entity0;
		Entity deliveryPointEntity = entity1;

		ECSCore* pECS = ECSCore::GetInstance();

		const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);

		if (flagParentComponent.Attached)
		{
			Entity entityPlayer = flagParentComponent.Parent;

			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
			const TeamComponent& playerTeamComponent = pTeamComponents->GetConstData(entityPlayer);

			TeamComponent flagTeamComponent = {};
			
			bool validFlagDelivery = false;
			if (pTeamComponents->GetConstIf(flagEntity, flagTeamComponent))
			{
				validFlagDelivery = (flagTeamComponent.TeamIndex != playerTeamComponent.TeamIndex);
			}
			else
			{
				validFlagDelivery = true;
			}

			if (validFlagDelivery)
			{
				const TeamComponent& deliveryPointTeamComponent = pTeamComponents->GetConstData(deliveryPointEntity);

				if (playerTeamComponent.TeamIndex == deliveryPointTeamComponent.TeamIndex)
				{
					//Flag will be respawned, set HasBeenPickedUp to false
					FlagComponent flagComponent = pECS->GetComponent<FlagComponent>(flagEntity);
					flagComponent.HasBeenPickedUp = false;

					EventQueue::SendEvent<FlagDeliveredEvent>(FlagDeliveredEvent(flagEntity, entityPlayer, flagTeamComponent.TeamIndex, playerTeamComponent.TeamIndex));
				}
			}
		}
	};

	pECS->ScheduleJobASAP(job);
}

void ServerFlagSystem::InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ RW, DynamicCollisionComponent::Type() });
	componentAccesses.PushBack({ RW, PacketComponent<PacketFlagEdited>::Type() });
}

void ServerFlagSystem::InternalAddAdditionalAccesses(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ R, NetworkPositionComponent::Type() });
}

void ServerFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	//Check for flag respawn (only in CTF_TEAM_FLAG)
	if (Match::GetGameMode() == EGameMode::CTF_TEAM_FLAG)
	{
		const ComponentArray<ParentComponent>*	pParentComponents = pECS->GetComponentArray<ParentComponent>();
		const ComponentArray<TeamComponent>*	pTeamComponents = pECS->GetComponentArray<TeamComponent>();
		ComponentArray<FlagComponent>*	pFlagComponents = pECS->GetComponentArray<FlagComponent>();

		for (Entity flagEntity : m_Flags)
		{
			const ParentComponent& parentComponent = pParentComponents->GetConstData(flagEntity);

			if (!parentComponent.Attached)
			{
				FlagComponent& flagComponent = pFlagComponents->GetData(flagEntity);

				if (flagComponent.HasBeenPickedUp && EngineLoop::GetTimeSinceStart() > flagComponent.DroppedTimestamp + flagComponent.RespawnCooldown)
				{
					//Flag will be respawned, set HasBeenPickedUp to false
					flagComponent.HasBeenPickedUp = false;

					TeamComponent flagTeamComponent = {};

					if (pTeamComponents->GetConstIf(flagEntity, flagTeamComponent))
					{
						EventQueue::SendEvent<FlagRespawnEvent>(FlagRespawnEvent(flagEntity, flagTeamComponent.TeamIndex));
					}
					else
					{
						EventQueue::SendEvent<FlagRespawnEvent>(FlagRespawnEvent(flagEntity, UINT8_MAX));
					}
				}
			}
		}
	}
}

void ServerFlagSystem::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<ParentComponent>*			pParentComponents = pECS->GetComponentArray<ParentComponent>();
	const ComponentArray<NetworkPositionComponent>*	pNetworkPositionComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	const ComponentArray<RotationComponent>*		pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<OffsetComponent>*			pOffsetComponents = pECS->GetComponentArray<OffsetComponent>();
	ComponentArray<DynamicCollisionComponent>*		pDynamicCollisionComponents = pECS->GetComponentArray<DynamicCollisionComponent>();

	for (Entity flagEntity : m_Flags)
	{
		const ParentComponent& parentComponent = pParentComponents->GetConstData(flagEntity);

		if (parentComponent.Attached)
		{
			const NetworkPositionComponent& parentPositionComponent = pNetworkPositionComponents->GetConstData(parentComponent.Parent);
			const RotationComponent& parentRotationComponent		= pRotationComponents->GetConstData(parentComponent.Parent);

			DynamicCollisionComponent& flagCollisionComponent	= pDynamicCollisionComponents->GetData(flagEntity);
			const OffsetComponent& flagOffsetComponent			= pOffsetComponents->GetConstData(flagEntity);

			glm::vec3 flagPosition;
			glm::quat flagRotation;

			CalculateAttachedFlagPosition(
				flagPosition,
				flagRotation,
				flagOffsetComponent.Offset,
				parentPositionComponent.Position,
				parentRotationComponent.Quaternion);

			PxTransform transform;
			transform.p.x = flagPosition.x;
			transform.p.y = flagPosition.y;
			transform.p.z = flagPosition.z;

			transform.q.x = flagRotation.x;
			transform.q.y = flagRotation.y;
			transform.q.z = flagRotation.z;
			transform.q.w = flagRotation.w;

			flagCollisionComponent.pActor->setKinematicTarget(transform);
		}
	}
}
