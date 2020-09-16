#include "Game/ECS/Rendering/Camera.h"

#include "Game/ECS/Physics/Transform.h"

namespace LambdaEngine
{
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
			.PrevProjection = VPMatrices.Projection,
			.PrevView = VPMatrices.View
		};

		m_VPMatrices.PushBack(VPMatrices, entity);
	}

	void CameraHandler::CreateCameraProperties(Entity entity, const glm::vec2& jitter)
	{
		m_CameraProperties.PushBack({ jitter }, entity);
		RegisterComponent(entity, CameraProperties::s_TID);
	}
}
