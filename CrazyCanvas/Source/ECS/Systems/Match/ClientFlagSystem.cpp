#include "ECS/Systems/Match/ClientFlagSystem.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Physics/CollisionGroups.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

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

		ParentComponent& flagParentComponent				= pECS->GetComponent<ParentComponent>(flagEntity);
		OffsetComponent& flagOffsetComponent				= pECS->GetComponent<OffsetComponent>(flagEntity);

		//Set Flag Carrier (Parent)
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
	UNREFERENCED_VARIABLE(entity0);
	UNREFERENCED_VARIABLE(entity1);

	//Handle Flag Collision
	LOG_WARNING("FLAG COLLIDED Client");
}

void ClientFlagSystem::InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses)
{
	using namespace LambdaEngine;
	componentAccesses.PushBack({ R, PacketComponent<FlagEditedPacket>::Type() });
}

void ClientFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	if (!m_Flags.Empty())
	{
		Entity flagEntity = m_Flags[0];

		PacketComponent<FlagEditedPacket>& flagPacketComponent = pECS->GetComponent<PacketComponent<FlagEditedPacket>>(flagEntity);
		const TArray<FlagEditedPacket>& flagEditedPackets = flagPacketComponent.GetPacketsReceived();

		for (const FlagEditedPacket& editedPacket : flagEditedPackets)
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

		const ParentComponent& parentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);

		if (parentComponent.Attached)
		{
			const PositionComponent& parentPositionComponent = pECS->GetComponent<PositionComponent>(parentComponent.Parent);
			const RotationComponent& parentRotationComponent = pECS->GetComponent<RotationComponent>(parentComponent.Parent);

			const OffsetComponent& flagOffsetComponent	= pECS->GetConstComponent<OffsetComponent>(flagEntity);
			PositionComponent& flagPositionComponent	= pECS->GetComponent<PositionComponent>(flagEntity);
			RotationComponent& flagRotationComponent	= pECS->GetComponent<RotationComponent>(flagEntity);

			flagPositionComponent.Position		= parentPositionComponent.Position;
			flagRotationComponent.Quaternion	= flagRotationComponent.Quaternion;
		}
	}
}
