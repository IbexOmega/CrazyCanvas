#include "ECS/ComponentManager.h"

using namespace LambdaEngine;

LambdaEngine::ComponentManager::ComponentManager()
{
}

LambdaEngine::ComponentManager::~ComponentManager()
{
	for (IComponentArray* compArr : m_ComponentArrays)
		SAFEDELETE(compArr);
	m_ComponentArrays.Clear();
}

void LambdaEngine::ComponentManager::EntityDeleted(Entity entity)
{
	for (IComponentArray* compArr : m_ComponentArrays)
		compArr->EntityDestroyed(entity);
}
