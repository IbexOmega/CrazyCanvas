#include "ECS/Systems/Match/ServerFlagSystem.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Physics/CollisionGroups.h"

#include "Resources/ResourceManager.h"

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
		{ ComponentPermissions::RW,	StaticCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	OffsetComponent::Type() }
	};

	job.Function = [flagEntity, playerEntity]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		const CharacterColliderComponent& playerCollisionComponent	= pECS->GetConstComponent<CharacterColliderComponent>(playerEntity);

		StaticCollisionComponent& flagCollisionComponent	= pECS->GetComponent<StaticCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent				= pECS->GetComponent<ParentComponent>(flagEntity);
		OffsetComponent& flagOffsetComponent				= pECS->GetComponent<OffsetComponent>(flagEntity);

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
	};

	pECS->ScheduleJobASAP(job);

	//Send Packet to el Clients
}

void ServerFlagSystem::OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::RW,	StaticCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	PositionComponent::Type() }
	};

	job.Function = [flagEntity, dropPosition]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		StaticCollisionComponent& flagCollisionComponent	= pECS->GetComponent<StaticCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent				= pECS->GetComponent<ParentComponent>(flagEntity);
		PositionComponent& flagPositionComponent			= pECS->GetComponent<PositionComponent>(flagEntity);

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

		//Set Flag Carrier (Parent)
		flagParentComponent.Attached	= false;
		flagParentComponent.Parent		= UINT32_MAX;

		//Set Position
		flagPositionComponent.Position	= dropPosition;
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
	componentAccesses.PushBack({ R, StaticCollisionComponent::Type() });
}

void ServerFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (!m_Flags.Empty())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		Entity flagEntity = m_Flags[0];

		const PositionComponent& flagPositionComponent		= pECS->GetConstComponent<PositionComponent>(flagEntity);
		const RotationComponent& flagRotationComponent		= pECS->GetConstComponent<RotationComponent>(flagEntity);

		StaticCollisionComponent& flagCollisionComponent	= pECS->GetComponent<StaticCollisionComponent>(flagEntity);

		PxTransform transform;
		transform.p.x = flagPositionComponent.Position.x;
		transform.p.y = flagPositionComponent.Position.y;
		transform.p.z = flagPositionComponent.Position.z;

		transform.q.x = flagRotationComponent.Quaternion.x;
		transform.q.y = flagRotationComponent.Quaternion.y;
		transform.q.z = flagRotationComponent.Quaternion.z;
		transform.q.w = flagRotationComponent.Quaternion.w;

		flagCollisionComponent.pActor->setGlobalPose(transform);
	}
}
