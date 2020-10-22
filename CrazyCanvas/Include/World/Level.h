#pragma once

#include "LambdaEngine.h"

#include "LevelModule.h"
#include "LevelObjectCreator.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/String.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

struct LevelCreateDesc
{
	LambdaEngine::String Name	=	"";
	LambdaEngine::TArray<LevelModule*>		LevelModules;
};

class Level : public LambdaEngine::IClientEntityAccessor
{
	struct LevelEntitiesOfType
	{
		LambdaEngine::TArray<uint64> SaltUIDs;
		LambdaEngine::TArray<LambdaEngine::Entity> Entities;
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>> ChildEntities;
	};

public:
	DECL_UNIQUE_CLASS(Level);

	Level();
	~Level();

	bool Init(const LevelCreateDesc* pDesc);

	// This should be used to create server entities:
	// Server & Client Loads Level, Client only loads clientside level objects,
	// Server then sends packages about server side entities that need to be created in the client, those should use this method
	// This method should delegate to LevelObjectCreator
	bool CreateObject(ESpecialObjectType specialObjectType, void* pData);

	LambdaEngine::Entity* GetEntities(ESpecialObjectType specialObjectType, uint32& countOut);

private:
	virtual LambdaEngine::Entity GetEntityPlayer(uint64 saltUID) override;

private:
	LambdaEngine::String m_Name = "";
	LambdaEngine::THashTable<ESpecialObjectType, uint32> m_EntityTypeMap;
	LambdaEngine::TArray<LevelEntitiesOfType> m_Entities;
};