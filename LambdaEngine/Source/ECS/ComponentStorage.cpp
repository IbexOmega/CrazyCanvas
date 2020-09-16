#include "ECS/ComponentStorage.h"

using namespace LambdaEngine;

LambdaEngine::ComponentStorage::~ComponentStorage()
{
	for (IComponentArray* compArr : m_ComponentArrays)
		SAFEDELETE(compArr);
	m_ComponentArrays.Clear();
}

void LambdaEngine::ComponentStorage::EntityDeleted(Entity entity)
{
	for (IComponentArray* compArr : m_ComponentArrays)
		compArr->EntityDestroyed(entity);
}
