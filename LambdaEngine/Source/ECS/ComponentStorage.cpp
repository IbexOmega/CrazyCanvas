#include "ECS/ComponentStorage.h"

namespace LambdaEngine
{
	ComponentStorage::~ComponentStorage()
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			SAFEDELETE(compArr);
		m_ComponentArrays.Clear();
	}

	void ComponentStorage::EntityDeleted(Entity entity)
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			compArr->EntityDestroyed(entity);
	}
}
