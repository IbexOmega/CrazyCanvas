#pragma once

#include "ECS/EntitySubscriber.h"
#include "Game/ECS/Physics/Transform.h"
#include "Game/ECS/Rendering/Camera.h"

namespace LambdaEngine
{
	class CameraComponents : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final {
			return {Position, Rotation, ViewProjectionMatrices};
		}

	public:
		ComponentAccess Position				= {NDA, g_TIDPosition};
		ComponentAccess Rotation				= {NDA, g_TIDRotation};
		ComponentAccess ViewProjectionMatrices	= {NDA, g_TIDViewProjectionMatrices};
		ComponentAccess CameraProperties		= {NDA, g_TIDCameraProperties};
	};
}
