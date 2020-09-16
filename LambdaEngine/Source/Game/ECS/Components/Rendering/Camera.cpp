#include "Game/ECS/Components/Rendering/Camera.h"

#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine
{
	INIT_COMPONENT(ViewProjectionMatrices);
	INIT_COMPONENT(CameraProperties);

	CameraHandler::CameraHandler()
		:ComponentHandler(TID(CameraHandler))
	{
		ComponentHandlerRegistration handlerReg = {};
		handlerReg.ComponentRegistrations = {
			{ViewProjectionMatrices::s_TID, &m_VPMatrices},
			{CameraProperties::s_TID, &m_CameraProperties}
		};

		RegisterHandler(handlerReg);
	}

	void CameraHandler::CreateViewProjectionMatrices(Entity entity, const ViewProjectionDesc& matricesDesc)
	{
		ViewProjectionMatrices VPMatrices = {
			.Projection = glm::perspective(glm::radians(matricesDesc.FOVDegrees), matricesDesc.Width / matricesDesc.Height, matricesDesc.NearPlane, matricesDesc.FarPlane),
			.View = glm::lookAt(matricesDesc.Position, matricesDesc.Position + matricesDesc.Direction, g_DefaultUp),
		};

		m_VPMatrices.PushBack(VPMatrices, entity);
	}

	void CameraHandler::CreateCameraProperties(Entity entity, const glm::vec2& jitter)
	{
		m_CameraProperties.PushBack({ jitter }, entity);
		RegisterComponent(entity, CameraProperties::s_TID);
	}
}
