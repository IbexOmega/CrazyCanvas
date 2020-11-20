#include "ECS/Systems/Match/ClientFlagSystem.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"

#include "Physics/CollisionGroups.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketFlagEdited.h"

ClientFlagSystem::ClientFlagSystem()
{
}

ClientFlagSystem::~ClientFlagSystem()
{
}

void ClientFlagSystem::OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity)
{
	using namespace LambdaEngine;
	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::R,	CharacterColliderComponent::Type() },
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	OffsetComponent::Type() }
	};

	job.Function = [flagEntity, playerEntity]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		const CharacterColliderComponent& playerCollisionComponent = pECS->GetConstComponent<CharacterColliderComponent>(playerEntity);
		OffsetComponent& flagOffsetComponent = pECS->GetComponent<OffsetComponent>(flagEntity);
		ParentComponent& flagParentComponent = pECS->GetComponent<ParentComponent>(flagEntity);

		// Attach Flag to player
		flagParentComponent.Attached	= true;
		flagParentComponent.Parent		= playerEntity;

		//Set Flag Offset
		const physx::PxBounds3& playerBoundingBox = playerCollisionComponent.pController->getActor()->getWorldBounds();
		flagOffsetComponent.Offset = glm::vec3(0.0f, playerBoundingBox.getDimensions().y / 2.0f, 0.0f);
	};

	pECS->ScheduleJobASAP(job);
}

void ClientFlagSystem::OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ ComponentPermissions::RW,	ParentComponent::Type() },
		{ ComponentPermissions::RW,	PositionComponent::Type() }
	};

	job.Function = [flagEntity, dropPosition]()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		ParentComponent& flagParentComponent		= pECS->GetComponent<ParentComponent>(flagEntity);
		PositionComponent& flagPositionComponent	= pECS->GetComponent<PositionComponent>(flagEntity);
		AnimationAttachedComponent& flagAnimAttachedComponent = pECS->GetComponent<AnimationAttachedComponent>(flagEntity);

		// Reset Flag orientation
		flagAnimAttachedComponent.Transform = glm::mat4(1.0f);

		//Set Flag Carrier (None)
		flagParentComponent.Attached	= false;
		flagParentComponent.Parent		= UINT32_MAX;

		//Set Position
		flagPositionComponent.Position	= dropPosition;
	};

	pECS->ScheduleJobASAP(job);
}

void ClientFlagSystem::OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//For now, we don't handle Player-Flag collisions on the client (the flag doesn't even have a collider)

	UNREFERENCED_VARIABLE(entity0);
	UNREFERENCED_VARIABLE(entity1);

	LOG_ERROR("Client: PLAYER-FLAG COLLISION");
}

void ClientFlagSystem::OnDeliveryPointFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//For now, we don't handle Base-Flag collisions on the client (the base doesn't even exist)

	UNREFERENCED_VARIABLE(entity0);
	UNREFERENCED_VARIABLE(entity1);

	LOG_ERROR("Client: BASE-FLAG COLLISION");
}

void ClientFlagSystem::InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ R, PacketComponent<PacketFlagEdited>::Type() });
}

void ClientFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
	const ComponentArray<OffsetComponent>* pOffsetComponents = pECS->GetComponentArray<OffsetComponent>();
	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

	for (Entity flagEntity : m_Flags)
	{
		const ParentComponent& parentComponent = pParentComponents->GetConstData(flagEntity);

		if (parentComponent.Attached)
		{
			const PositionComponent& parentPositionComponent = pPositionComponents->GetConstData(parentComponent.Parent);
			const RotationComponent& parentRotationComponent = pRotationComponents->GetConstData(parentComponent.Parent);

			const OffsetComponent& flagOffsetComponent	= pOffsetComponents->GetConstData(flagEntity);
			PositionComponent& flagPositionComponent	= pPositionComponents->GetData(flagEntity);
			RotationComponent& flagRotationComponent	= pRotationComponents->GetData(flagEntity);

			CalculateAttachedFlagPosition(
				flagPositionComponent.Position,
				flagRotationComponent.Quaternion,
				flagOffsetComponent.Offset,
				parentPositionComponent.Position,
				parentRotationComponent.Quaternion);
		}
	}
}

void ClientFlagSystem::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	for (Entity flagEntity : m_Flags)
	{
		PacketComponent<PacketFlagEdited>& flagPacketComponent = pECS->GetComponent<PacketComponent<PacketFlagEdited>>(flagEntity);
		const TArray<PacketFlagEdited>& flagEditedPackets = flagPacketComponent.GetPacketsReceived();

		for (const PacketFlagEdited& editedPacket : flagEditedPackets)
		{
			switch (editedPacket.FlagPacketType)
			{
			case EFlagPacketType::FLAG_PACKET_TYPE_PICKED_UP:
				OnFlagPickedUp(MultiplayerUtils::GetEntity(editedPacket.PickedUpNetworkUID), flagEntity);
				break;
			case EFlagPacketType::FLAG_PACKET_TYPE_DROPPED:
				OnFlagDropped(flagEntity, editedPacket.DroppedPosition);
				break;
			}
		}
	}
}
