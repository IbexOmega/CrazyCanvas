#include "ECS/Systems/Match/ClientShowerSystem.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Multiplayer/Packet/PacketResetPlayerTexture.h"

#include "RenderStages/PaintMaskRenderer.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"

ClientShowerSystem::ClientShowerSystem()
{
}

ClientShowerSystem::~ClientShowerSystem()
{
}

void ClientShowerSystem::OnPlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);
	UNREFERENCED_VARIABLE(entity1);
}

void ClientShowerSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void ClientShowerSystem::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PacketComponent<PacketResetPlayerTexture>>* pPacketResetPlayerTexture = pECS->GetComponentArray<PacketComponent<PacketResetPlayerTexture>>();

	for (Entity entity : m_PlayerEntities)
	{
		PacketComponent<PacketResetPlayerTexture>& packets = pPacketResetPlayerTexture->GetData(entity);

		if (!packets.GetPacketsReceived().IsEmpty())
		{
			PaintMaskRenderer::ResetServer(entity);
			PaintMaskRenderer::ResetServer(pECS->GetComponent<ChildComponent>(entity).GetEntityWithTag("weapon"));
		}
	}
}