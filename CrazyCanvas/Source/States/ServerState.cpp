#include "States/ServerState.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"
#include "Application/API/Events/EventQueue.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Math/Random.h"

#include <argh/argh.h>

#include "Game/ECS/Systems/Networking/Server/ServerSystem.h"

#include "World/Level.h"
#include "World/LevelManager.h"

using namespace LambdaEngine;

ServerState::~ServerState()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	SAFEDELETE(m_pLevel);
}

void ServerState::Init()
{
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &ServerState::OnClientConnected);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	//NetworkDiscovery::EnableServer("Crazy Canvas", 4444, this);

	// Load scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
		MultiplayerUtils::RegisterClientEntityAccessor(m_pLevel);
	}

	ServerSystem::GetInstance().Start();
}

bool ServerState::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

bool ServerState::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();

	IClient* pClient = event.pClient;

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	uint32 otherPlayerCount = 0;
	Entity* pOtherPlayerEntities = m_pLevel->GetEntities(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, otherPlayerCount);

	for (uint32 i = 0; i < otherPlayerCount; i++)
	{
		Entity otherPlayerEntity = pOtherPlayerEntities[i];
		const PositionComponent& positionComponent = pPositionComponents->GetData(otherPlayerEntity);

		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
		BinaryEncoder encoder3(pPacket);
		encoder3.WriteBool(false);
		encoder3.WriteInt32((int32)otherPlayerEntity);
		encoder3.WriteVec3(positionComponent.Position);
		pClient->SendReliable(pPacket, nullptr);
	}

	glm::vec3 position(0.0f, 2.0f, 0.0f);

	CreatePlayerDesc createPlayerDesc =
	{
		.pClient	= pClient,
		.Position	= position,
		.Forward	= glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
		.Scale		= glm::vec3(0.01f),
	};

	m_pLevel->CreateObject(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, &createPlayerDesc);
	return true;
}

void ServerState::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void ServerState::OnNetworkDiscoveryPreTransmit(const BinaryEncoder& encoder)
{
	UNREFERENCED_VARIABLE(encoder);
}
