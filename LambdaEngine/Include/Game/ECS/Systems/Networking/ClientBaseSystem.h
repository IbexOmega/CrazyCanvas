#pragma once

#include "ECS/System.h"

namespace LambdaEngine
{
	class ClientBaseSystem : public System
	{
		friend class ServerSystem;

	public:
		DECL_UNIQUE_CLASS(ClientBaseSystem);
		virtual ~ClientBaseSystem();

		void Tick(Timestamp deltaTime) override;


	protected:
		ClientBaseSystem();

		virtual void TickMainThread(Timestamp deltaTime) = 0;
		virtual void FixedTickMainThread(Timestamp deltaTime) = 0;

		//void PlayerUpdate(Entity entity, const GameState& gameState);

	private:
		
	};
}