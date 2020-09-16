#include "Game/ECS/Components/Rendering/Camera.h"

#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine
{
	INIT_COMPONENT(ViewProjectionMatrices);
	INIT_COMPONENT(CameraProperties);

	ViewProjectionMatrices CreateViewProjectionMatrices(Entity entity, const ViewProjectionDesc& matricesDesc)
	{
		return ViewProjectionMatrices {
			.Projection = glm::perspective(glm::radians(matricesDesc.FOVDegrees), matricesDesc.Width / matricesDesc.Height, matricesDesc.NearPlane, matricesDesc.FarPlane),
			.View = glm::lookAt(matricesDesc.Position, matricesDesc.Position + matricesDesc.Direction, g_DefaultUp)
		};
	}
}
