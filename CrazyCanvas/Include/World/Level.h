#pragma once

#include "LambdaEngine.h"

#include "LevelModule.h"
#include "LevelObjectCreator.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/String.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Threading/API/SpinLock.h"

#include "ECS/ComponentOwner.h"

#include "ECS/Components/World/LevelRegisteredComponent.h"

struct LevelCreateDesc
{
	LambdaEngine::String Name	=	"";
	LambdaEngine::TArray<LevelModule*>		LevelModules;
};

class Level : public LambdaEngine::ComponentOwner
{
public:
	DECL_UNIQUE_CLASS(Level);

	Level();
	~Level();

	bool Init(const LevelCreateDesc* pDesc);

	bool CreateObject(ELevelObjectType levelObjectType, const void* pData, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities);
	bool DeleteObject(LambdaEngine::Entity entity);
	bool RegisterObjectDeletion(LambdaEngine::Entity entity);

	LambdaEngine::TArray<LambdaEngine::Entity> GetEntities(ELevelObjectType levelObjectType);
	ELevelObjectType GetLevelObjectType(LambdaEngine::Entity entity) const;

private:
	void LevelRegisteredDestructor(LevelRegisteredComponent& levelRegisteredComponent, LambdaEngine::Entity entity);

private:
	LambdaEngine::String m_Name = "";
	LambdaEngine::SpinLock m_SpinLock;

	LambdaEngine::THashTable<LambdaEngine::Entity, ELevelObjectType> m_EntityToLevelObjectTypeMap;
	LambdaEngine::THashTable<ELevelObjectType, uint32> m_EntityTypeMap;

	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>> m_LevelEntities;
};