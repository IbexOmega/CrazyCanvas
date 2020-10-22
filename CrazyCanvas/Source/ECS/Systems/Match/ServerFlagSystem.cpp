#include "ECS/Systems/Match/ServerFlagSystem.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Physics/CollisionGroups.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

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
		{ ComponentPermissions::R,	CharacterColliderComponent::Type() },
		{ ComponentPermissions::RW,	DynamicCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	OffsetComponent::Type() },
		{ ComponentPermissions::RW,	PacketComponent<FlagEditedPacket>::Type() },
	};

	job.Function = [flagEntity, playerEntity]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		const CharacterColliderComponent& playerCollisionComponent	= pECS->GetConstComponent<CharacterColliderComponent>(playerEntity);

		DynamicCollisionComponent& flagCollisionComponent		= pECS->GetComponent<DynamicCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent					= pECS->GetComponent<ParentComponent>(flagEntity);
		OffsetComponent& flagOffsetComponent					= pECS->GetComponent<OffsetComponent>(flagEntity);
		PacketComponent<FlagEditedPacket>& flagPacketComponent	= pECS->GetComponent<PacketComponent<FlagEditedPacket>>(flagEntity);

		PxShape* pFlagShape;
		flagCollisionComponent.pActor->getShapes(&pFlagShape, 1);
		pFlagShape->acquireReference();
		flagCollisionComponent.pActor->detachShape(*pFlagShape);

		//Update Collision Group
		PxFilterData filterData;
		filterData.word0 = (PxU32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG;
		filterData.word1 = (PxU32)FLAG_CARRIED_COLLISION_MASK;
		pFlagShape->setSimulationFilterData(filterData);
		pFlagShape->setQueryFilterData(filterData);

		flagCollisionComponent.pActor->attachShape(*pFlagShape);
		pFlagShape->release();

		//Set Flag Carrier (Parent)
		flagParentComponent.Attached	= true;
		flagParentComponent.Parent		= playerEntity;

		//Set Flag Offset
		const physx::PxBounds3& playerBoundingBox = playerCollisionComponent.pController->getActor()->getWorldBounds();
		flagOffsetComponent.Offset = glm::vec3(0.0f, playerBoundingBox.getDimensions().y / 2.0f, 0.0f);

		//Send Packet
		FlagEditedPacket packet	= {};
		packet.FlagPacketType	= EFlagPacketType::FLAG_PACKET_TYPE_PICKED_UP;
		packet.NetworkUID		= MultiplayerUtils::GetNetworkUID(playerEntity);
		flagPacketComponent.SendPacket(packet);
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
		{ ComponentPermissions::RW,	DynamicCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	PositionComponent::Type() },
		{ ComponentPermissions::RW,	PacketComponent<FlagEditedPacket>::Type() },
	};

	job.Function = [flagEntity, dropPosition]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		DynamicCollisionComponent& flagCollisionComponent		= pECS->GetComponent<DynamicCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent					= pECS->GetComponent<ParentComponent>(flagEntity);
		PositionComponent& flagPositionComponent				= pECS->GetComponent<PositionComponent>(flagEntity);
		PacketComponent<FlagEditedPacket>& flagPacketComponent	= pECS->GetComponent<PacketComponent<FlagEditedPacket>>(flagEntity);

		PxShape* pFlagShape;
		flagCollisionComponent.pActor->getShapes(&pFlagShape, 1);
		pFlagShape->acquireReference();
		flagCollisionComponent.pActor->detachShape(*pFlagShape);

		//Update Collision Group
		PxFilterData filterData;
		filterData.word0 = (PxU32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG;
		filterData.word1 = (PxU32)FLAG_DROPPED_COLLISION_MASK;
		pFlagShape->setSimulationFilterData(filterData);
		pFlagShape->setQueryFilterData(filterData);

		flagCollisionComponent.pActor->attachShape(*pFlagShape);
		pFlagShape->release();

		//Set Flag Carrier (None)
		flagParentComponent.Attached	= false;
		flagParentComponent.Parent		= UINT32_MAX;

		//Set Position
		flagPositionComponent.Position	= dropPosition;

		//Send Packet
		FlagEditedPacket packet	= {};
		packet.FlagPacketType	= EFlagPacketType::FLAG_PACKET_TYPE_DROPPED;
		packet.DroppedPosition	= dropPosition;
		flagPacketComponent.SendPacket(packet);
	};

	pECS->ScheduleJobASAP(job);
}

void ServerFlagSystem::OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//Handle Flag Collision
	LOG_WARNING("FLAG COLLIDED Server");

	OnFlagPickedUp(entity1, entity0);
}

void ServerFlagSystem::InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ RW, DynamicCollisionComponent::Type() });
	componentAccesses.PushBack({ RW, PacketComponent<FlagEditedPacket>::Type() });
}

void ServerFlagSystem::InternalAddAdditionalAccesses(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ R, NetworkPositionComponent::Type() });
}

void ServerFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (!m_Flags.Empty())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity flagEntity = m_Flags[0];

		const ParentComponent& parentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);

		if (parentComponent.Attached)
		{
			const NetworkPositionComponent& parentPositionComponent = pECS->GetComponent<NetworkPositionComponent>(parentComponent.Parent);
			const RotationComponent& parentRotationComponent		= pECS->GetComponent<RotationComponent>(parentComponent.Parent);

			DynamicCollisionComponent& flagCollisionComponent	= pECS->GetComponent<DynamicCollisionComponent>(flagEntity);
			const OffsetComponent& flagOffsetComponent			= pECS->GetConstComponent<OffsetComponent>(flagEntity);

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
