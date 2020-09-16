#pragma once

#include "ECS/EntitySubscriber.h"
#include "Game/ECS/Physics/Transform.h"

namespace LambdaEngine
{
	class TransformComponents : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final
		{
			return {m_Position, m_Scale, m_Rotation};
		}

	public:
		ComponentAccess m_Position    = {NDA, g_TIDPosition};
		ComponentAccess m_Scale       = {NDA, g_TIDScale};
		ComponentAccess m_Rotation    = {NDA, g_TIDRotation};
	};
}
