#include "ECS/Systems/Match/ServerFlagSystem.h"

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
		{ ComponentPermissions::R,	MeshComponent::Type() },
		{ ComponentPermissions::RW,	StaticCollisionComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	OffsetComponent::Type() }
	};

	job.Function = [flagEntity, playerEntity]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		const MeshComponent& playerMeshComponent			= pECS->GetConstComponent<MeshComponent>(playerEntity);

		StaticCollisionComponent& flagCollisionComponent	= pECS->GetComponent<StaticCollisionComponent>(flagEntity);
		ParentComponent& flagParentComponent				= pECS->GetComponent<ParentComponent>(flagEntity);
		OffsetComponent& flagOffsetComponent				= pECS->GetComponent<OffsetComponent>(flagEntity);

		PxShape* pFlagShape;
		flagCollisionComponent.pActor->getShapes(&pFlagShape, 1);
			
		//Update Collision Group
		PxFilterData filterData;
		filterData.word0 = (PxU32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG;
		filterData.word1 = (PxU32)FCollisionGroup::COLLISION_GROUP_NONE;
		pFlagShape->setSimulationFilterData(filterData);
		pFlagShape->setQueryFilterData(filterData);

		//Set Flag Carrier (Parent)
		flagParentComponent.Attached	= true;
		flagParentComponent.Parent		= playerEntity;

		//Set Flag Offset
		const Mesh* pMesh = ResourceManager::GetMesh(playerMeshComponent.MeshGUID);
		flagOffsetComponent.Offset		= glm::vec3(0.0f, pMesh->BoundingBox.Dimensions.y / 2.0f, 0.0f);
	};

	pECS->ScheduleJobASAP(job);
}

void ServerFlagSystem::OnFlagDropped()
{
}

void ServerFlagSystem::OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//Handle Flag Collision
	LOG_WARNING("FLAG COLLIDED Server");

	OnFlagPickedUp(entity1, entity0);
}

void ServerFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	//Check if the flag is in a base
}
