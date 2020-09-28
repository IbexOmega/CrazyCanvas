#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Containers/CCBuffer.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
	struct GameState
	{
		int32 SimulationTick	= -1;
		int8 DeltaForward		= 0;
		int8 DeltaLeft			= 0;
	};

	struct GameStateComparator
	{
		bool operator() (const GameState& lhs, const GameState& rhs) const
		{
			return lhs.SimulationTick < rhs.SimulationTick;
		}
	};

	class ClientRemoteSystem : public System, public IClientRemoteHandler, public IPacketListener
	{
		friend class ServerSystem;

	public:
		DECL_UNIQUE_CLASS(ClientRemoteSystem);
		virtual ~ClientRemoteSystem();

		void Tick(Timestamp deltaTime) override;

		void TickMainThread(Timestamp deltaTime);
		void FixedTickMainThread(Timestamp deltaTime);

		void PlayerUpdate(const GameState& gameState);

	protected:
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
		Entity m_Entity;
		int32 m_LastProcessedSimulationTick;

		//Temp, remove later plz
		glm::vec3 m_Color;

	private:
		static glm::vec3 s_StartPositions[10];
		static glm::vec3 s_StartColors[10];
	};
}