#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine
{
	class LAMBDA_API CameraSystem : public System
	{
	public:
		DECL_REMOVE_COPY(CameraSystem);
		DECL_REMOVE_MOVE(CameraSystem);
		~CameraSystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

		void MainThreadTick(Timestamp deltaTime);

	public:
		static CameraSystem& GetInstance() { return s_Instance; }

	private:
		CameraSystem() = default;

		void HandleInput(Timestamp deltaTime, Entity entity, PositionComponent& posComp, RotationComponent& rotComp, const FreeCameraComponent& freeCamComp);
		void RenderFrustum(Entity entity);

	private:
		IDVector	m_CameraEntities;
		bool		m_CIsPressed	= false;
		bool		m_MouseEnabled	= false;

		bool		m_VisbilityChanged = false;
		glm::ivec2	m_NewMousePos;

		THashTable<Entity, uint32> m_LineGroupEntityIDs;

	private:
		static CameraSystem	s_Instance;
	};
}