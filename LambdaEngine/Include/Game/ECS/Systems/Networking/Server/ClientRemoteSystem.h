#pragma once

#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Game/Multiplayer/GameState.h"

#include "Containers/CCBuffer.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
	class ClientRemoteSystem : public ClientBaseSystem, public IClientRemoteHandler, public IPacketListener
	{
		friend class ServerSystem;

	public:
		DECL_UNIQUE_CLASS(ClientRemoteSystem);
		virtual ~ClientRemoteSystem();

	protected:
		virtual void TickMainThread(Timestamp deltaTime) override;
		virtual void FixedTickMainThread(Timestamp deltaTime) override;
		virtual Entity GetEntityPlayer() const;

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;

		virtual void OnPacketDelivered(NetworkSegment* pPacket) override;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 retries) override;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 retries) override;

	private:
		ClientRemoteSystem();

	private:
		IDVector m_NetworkEntities;
		TSet<GameState, GameStateComparator> m_Buffer;
		GameState m_CurrentGameState;
		ClientRemoteBase* m_pClient;
		Entity m_EntityPlayer;

		//Temp, remove later plz
		glm::vec3 m_Color;

	private:
		static glm::vec3 s_StartPositions[10];
		static glm::vec3 s_StartColors[10];
	};
}