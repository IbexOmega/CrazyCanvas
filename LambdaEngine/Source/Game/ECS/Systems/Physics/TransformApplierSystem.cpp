#include "Game/ECS/Systems/Physics/TransformApplierSystem.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"
#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Collision.h"

namespace LambdaEngine
{
	TransformApplierSystem TransformApplierSystem::s_Instance;

	void TransformApplierSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_MatrixEntities,
				.ComponentAccesses =
				{
					{RW, CameraComponent::Type()}, {RW, ViewProjectionMatricesComponent::Type()},
					{R, PositionComponent::Type()}, {R, RotationComponent::Type()}
				}
			},
			{
				.pSubscriber = &m_VelocityEntities,
				.ComponentAccesses =
				{
					{RW, PositionComponent::Type()}, {R, VelocityComponent::Type()}
				},
				.ExcludedComponentTypes =
				{
					CharacterColliderComponent::Type(), DynamicCollisionComponent::Type()
				}
			}
		};
		systemReg.Phase = LAST_PHASE - 1;

		RegisterSystem(TYPE_NAME(TransformApplierSystem), systemReg);
	}

	void TransformApplierSystem::Tick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		m_Tick++;

		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<PositionComponent>* pPositionComponents				= pECS->GetComponentArray<PositionComponent>();
		const ComponentArray<VelocityComponent>* pVelocityComponents		= pECS->GetComponentArray<VelocityComponent>();
		const ComponentArray<RotationComponent>* pRotationComponents		= pECS->GetComponentArray<RotationComponent>();
		ComponentArray<CameraComponent>* pCameraComponents					= pECS->GetComponentArray<CameraComponent>();
		ComponentArray<ViewProjectionMatricesComponent>* pViewProjectionComponents	= pECS->GetComponentArray<ViewProjectionMatricesComponent>();

		for (Entity entity : m_VelocityEntities)
		{
			const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(entity);
			if (glm::length2(velocityComp.Velocity))
			{
				PositionComponent& positionComp = pPositionComponents->GetData(entity);
				positionComp.Position += velocityComp.Velocity * dt;
			}
		}

		for (Entity entity : m_MatrixEntities)
		{
			const PositionComponent& positionComp = pPositionComponents->GetConstData(entity);
			const RotationComponent& rotationComp = pRotationComponents->GetConstData(entity);
			CameraComponent& cameraComp = pCameraComponents->GetData(entity);
			ViewProjectionMatricesComponent& viewProjComp = pViewProjectionComponents->GetData(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const uint16 width	= window->GetWidth();
			const uint16 height	= window->GetHeight();

			// Generate jitter
			constexpr uint32 SAMPLES = 16;
			const uint64 sampleIndex = m_Tick % SAMPLES;
			glm::vec2 jitter = Math::Hammersley2D(sampleIndex, SAMPLES);
			jitter = (jitter * 2.0f) - 1.0f;
			cameraComp.Jitter.x = jitter.x;
			cameraComp.Jitter.y = jitter.y;
			cameraComp.Width	= float(width);
			cameraComp.Height	= float(height);

			viewProjComp.View	= glm::lookAt(positionComp.Position, positionComp.Position + GetForward(rotationComp.Quaternion), g_DefaultUp);
			cameraComp.ViewInv	= glm::inverse(viewProjComp.View);
			cameraComp.ProjectionInv = glm::inverse(viewProjComp.Projection);
		}
	}
}
