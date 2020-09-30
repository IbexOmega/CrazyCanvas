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

		// MoveFreeCamera translates and rotates a free camera
		void MoveFreeCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotComp, const FreeCameraComponent& freeCamComp);
		// MoveFPSCamera translates and rotates an FPS camera
		void MoveFPSCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotComp, const FPSControllerComponent& FPSComp);
		void RotateCamera(float32 dt, float32 mouseSpeedFactor, const glm::vec3& forward, glm::quat& rotation);
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