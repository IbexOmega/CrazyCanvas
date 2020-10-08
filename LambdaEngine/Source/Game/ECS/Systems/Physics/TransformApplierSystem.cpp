#include "Game/ECS/Systems/Physics/TransformApplierSystem.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"
#include "ECS/ECSCore.h"
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
				{
					{RW, CameraComponent::Type()}, {RW, ViewProjectionMatricesComponent::Type()},
					{R, PositionComponent::Type()}, {R, RotationComponent::Type()}
				},
				&m_MatrixEntities
			},
			{
				{
					{RW, PositionComponent::Type()}, {R, VelocityComponent::Type()}
				},
				{

				},
				{
					CharacterColliderComponent::Type()
				},
				&m_VelocityEntities
			}
		};
		systemReg.Phase = g_LastPhase - 1;

		RegisterSystem(systemReg);
	}

	void TransformApplierSystem::Tick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();

		ECSCore* pECS = ECSCore::GetInstance();
		auto* pPositionComponents		= pECS->GetComponentArray<PositionComponent>();
		const auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
		const auto* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		auto* pCameraComponents			= pECS->GetComponentArray<CameraComponent>();
		auto* pViewProjectionComponents = pECS->GetComponentArray<ViewProjectionMatricesComponent>();

		for (Entity entity : m_VelocityEntities)
		{
			const VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);
			if (glm::length2(velocityComp.Velocity))
			{
				PositionComponent& positionComp = pPositionComponents->GetData(entity);
				positionComp.Position += velocityComp.Velocity * dt;
			}
		}

		for (Entity entity : m_MatrixEntities)
		{
			const PositionComponent& positionComp = pPositionComponents->GetData(entity);
			const RotationComponent& rotationComp = pRotationComponents->GetData(entity);
			CameraComponent& cameraComp = pCameraComponents->GetData(entity);
			ViewProjectionMatricesComponent& viewProjComp = pViewProjectionComponents->GetData(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const uint16 width = window->GetWidth();
			const uint16 height = window->GetHeight();
			cameraComp.Jitter = glm::vec2((Random::Float32() - 0.5f) / (float)width, (Random::Float32() - 0.5f) / (float)height);

			viewProjComp.View = glm::lookAt(positionComp.Position, positionComp.Position + GetForward(rotationComp.Quaternion), g_DefaultUp);
			cameraComp.ViewInv = glm::inverse(viewProjComp.View);
			cameraComp.ProjectionInv = glm::inverse(viewProjComp.Projection);
		}
	}
}
