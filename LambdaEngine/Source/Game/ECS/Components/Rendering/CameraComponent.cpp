#include "Game/ECS/Components/Rendering/CameraComponent.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	INIT_COMPONENT(FreeCameraComponent);
	INIT_COMPONENT(CameraComponent);
	INIT_COMPONENT(ViewProjectionMatricesComponent);

	void CreateFreeCameraEntity(const CameraDesc& cameraDesc)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		FreeCameraComponent freeCamComp;
		pECS->AddComponent<FreeCameraComponent>(entity, freeCamComp);

		ViewProjectionMatricesComponent viewProjComp;
		viewProjComp.Projection = glm::perspective(glm::radians(cameraDesc.FOVDegrees), cameraDesc.Width / cameraDesc.Height, cameraDesc.NearPlane, cameraDesc.FarPlane);
		viewProjComp.View = glm::lookAt(cameraDesc.Position, cameraDesc.Position + cameraDesc.Direction, g_DefaultUp);
		pECS->AddComponent<ViewProjectionMatricesComponent>(entity, viewProjComp);

		CameraComponent camComp;
		pECS->AddComponent<CameraComponent>(entity, camComp);
		
		pECS->AddComponent<PositionComponent>(entity, PositionComponent{ .Position = cameraDesc.Position });
		pECS->AddComponent<RotationComponent>(entity, RotationComponent{ .Quaternion = glm::quatLookAtLH(cameraDesc.Direction, g_DefaultUp) });
		pECS->AddComponent<ScaleComponent>(entity, ScaleComponent{ .Scale = {1.f, 1.f, 1.f} });
	}
}
