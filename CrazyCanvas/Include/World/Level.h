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

struct LevelCreateDesc
{
	LambdaEngine::String Name	=	"";
	LambdaEngine::TArray<LevelModule*>		LevelModules;
};

class Level
{
public:
	DECL_UNIQUE_CLASS(Level);

	Level();
	~Level();

	bool Init(const LevelCreateDesc* pDesc);

	bool DeleteObject(LambdaEngine::Entity entity);
	bool CreateObject(ELevelObjectType levelObjectType, const void* pData, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities);

	LambdaEngine::TArray<LambdaEngine::Entity> GetEntities(ELevelObjectType levelObjectType);
	LambdaEngine::TArray< LambdaEngine::TArray<LambdaEngine::Entity>> GetChildEntities(ELevelObjectType levelObjectType);

private:
	//virtual LambdaEngine::Entity GetEntityPlayer(uint64 saltUID) override;

private:
	LambdaEngine::String m_Name = "";
	LambdaEngine::SpinLock m_SpinLock;

	LambdaEngine::THashTable<LambdaEngine::Entity, ELevelObjectType> m_EntityToLevelObjectTypeMap;
	LambdaEngine::THashTable<ELevelObjectType, uint32> m_EntityTypeMap;

	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>> m_LevelEntities;
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>> m_LevelChildEntities;
};