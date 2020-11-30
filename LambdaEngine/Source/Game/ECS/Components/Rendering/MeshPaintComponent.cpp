#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Rendering/EntityMaskManager.h"

namespace LambdaEngine
{
	MeshPaintComponent MeshPaint::CreateComponent(Entity entity)
	{
		EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), nullptr);

		return {};
	}
}