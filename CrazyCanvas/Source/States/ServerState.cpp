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

#include "Teams/TeamHelper.h"

#include "Game/Multiplayer/Server/ServerSystem.h"

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
	EventQueue::RegisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	m_ServerName = "Crazy Canvas Server";

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

bool ServerState::OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event)
{
	BinaryEncoder* pEncoder = event.pEncoder;
	ServerBase* pServer = event.pServer;

	pEncoder->WriteUInt8(pServer->GetClientCount());
	pEncoder->WriteString(m_ServerName);
	pEncoder->WriteString("This Is The Map Name");

	return true;
}

bool ServerState::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();

	IClient* pClient = event.pClient;

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	uint32 otherPlayerCount = 0;
	Entity* pOtherPlayerEntities = m_pLevel->GetEntities(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, otherPlayerCount);

	for (uint32 i = 0; i < otherPlayerCount; i++)
	{
		Entity otherPlayerEntity = pOtherPlayerEntities[i];
		const PositionComponent& positionComponent = pPositionComponents->GetConstData(otherPlayerEntity);
		const RotationComponent& rotationComponent = pRotationComponents->GetConstData(otherPlayerEntity);
		const TeamComponent& teamComponent = pTeamComponents->GetConstData(otherPlayerEntity);

		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
		BinaryEncoder encoder(pPacket);
		encoder.WriteBool(false);
		encoder.WriteInt32((int32)otherPlayerEntity);
		encoder.WriteVec3(positionComponent.Position);
		encoder.WriteVec3(GetForward(rotationComponent.Quaternion));
		encoder.WriteUInt32(teamComponent.TeamIndex);
		pClient->SendReliable(pPacket, nullptr);
	}

	static glm::vec3 position(0.0f, 10.0f, 0.0f);
	static uint32 teamIndex = 0;

	CreatePlayerDesc createPlayerDesc =
	{
		.pClient		= pClient,
		.Position		= position,
		.Forward		= glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
		.Scale			= glm::vec3(1.0f),
		.TeamIndex		= teamIndex,
	};

	m_pLevel->CreateObject(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, &createPlayerDesc);

	teamIndex = (teamIndex + 1) % 2;
	return true;
}

void ServerState::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}
