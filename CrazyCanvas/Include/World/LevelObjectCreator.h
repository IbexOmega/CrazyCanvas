#pragma once
#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include "Resources/ResourceLoader.h"

#include "ECS/Entity.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#include "Lobby/Player.h"

#define PLAYER_CAPSULE_HEIGHT 1.6f
#define PLAYER_CAPSULE_RADIUS 0.325f

namespace LambdaEngine
{
	class IClient;

	struct CameraDesc;
};

/*
* ELevelObjectType
*/

enum class ELevelObjectType : uint8
{
	LEVEL_OBJECT_TYPE_NONE							= 0,
	LEVEL_OBJECT_TYPE_STATIC_GEOMETRY				= 1,
	LEVEL_OBJECT_TYPE_DIR_LIGHT						= 2,
	LEVEL_OBJECT_TYPE_POINT_LIGHT					= 3,
	LEVEL_OBJECT_TYPE_PLAYER_SPAWN					= 4,
	LEVEL_OBJECT_TYPE_PLAYER						= 5,
	LEVEL_OBJECT_TYPE_PLAYER_SPECTATOR				= 6,
	LEVEL_OBJECT_TYPE_FLAG_SPAWN					= 7,
	LEVEL_OBJECT_TYPE_FLAG							= 8,
	LEVEL_OBJECT_TYPE_FLAG_DELIVERY_POINT			= 9,
	LEVEL_OBJECT_TYPE_PROJECTILE					= 10,
	LEVEL_OBJECT_TYPE_KILL_PLANE					= 11,
	LEVEL_OBJECT_TYPE_PLAYER_JAIL					= 12,
	LEVEL_OBJECT_TYPE_SPECTATE_MAP_POINT			= 13,
	LEVEL_OBJECT_TYPE_GLOBAL_LIGHT_PROBE			= 14,
	LEVEL_OBJECT_TYPE_PARTICLE_SHOWER				= 15,
	LEVEL_OBJECT_TYPE_TEAM_INDICATOR				= 16,
	LEVEL_OBJECT_TYPE_STATIC_GEOMETRY_NO_COLLIDER	= 17,
};

/*
* CreateFlagDesc
*/

struct CreateFlagDesc
{
	int32					NetworkUID		= -1;
	LambdaEngine::Entity	ParentEntity	= UINT32_MAX;
	glm::vec3				Position		= glm::vec3(0.0f);
	glm::vec3				Scale			= glm::vec3(1.0f);
	glm::quat				Rotation		= glm::quat();
	uint8					TeamIndex		= UINT8_MAX;
};

/*
* CreatePlayerDesc
*/

struct CreatePlayerDesc
{
	const Player*						pPlayer				= nullptr;
	bool								IsLocal				= false;
	int32								PlayerNetworkUID	= -1;
	int32								WeaponNetworkUID	= -1;
	glm::vec3							Position			= glm::vec3(0.0f);
	glm::vec3							Forward				= glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3							Scale				= glm::vec3(1.0f);
	const LambdaEngine::CameraDesc*		pCameraDesc			= nullptr;
};

/*
* CreateProjectileDesc
*/

struct CreateProjectileDesc
{
	EAmmoType	AmmoType;
	glm::vec3	FirePosition;
	glm::vec3	InitalVelocity;
	int32		PlayerFiredNetworkUID = -1;
	uint8		TeamIndex;
	LambdaEngine::Entity			WeaponOwner;
	LambdaEngine::CollisionCallback	Callback;
	uint32 Angle = 0;
};

/*
* LevelObjectCreator
*/

class LevelObjectCreator
{
	typedef ELevelObjectType(*LevelObjectCreateByPrefixFunc)(const LambdaEngine::LevelObjectOnLoad&, LambdaEngine::TArray<LambdaEngine::Entity>&, const glm::vec3&);
	typedef bool(*LevelObjectCreateByTypeFunc)(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>&,
		LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>&);

public:
	DECL_STATIC_CLASS(LevelObjectCreator);

	static bool Init();

	static LambdaEngine::Entity CreateDirectionalLight(const LambdaEngine::LoadedDirectionalLight& directionalLight, const glm::vec3& translation);
	static LambdaEngine::Entity CreatePointLight(const LambdaEngine::LoadedPointLight& pointLight, const glm::vec3& translation);
	static LambdaEngine::Entity CreateStaticGeometry(const LambdaEngine::MeshComponent& meshComponent, const glm::vec3& translation);

	/*
	*	Special Objects are entities that can be created at level load time or later, they are more than general than lights and geometry in the sense
	*	that as long as a create function exists, it can be created. This means that in order to add a new type of Special Object, one just has to add a new
	*	a new create function for that Special Object.
	*
	*	The create functions can then either be called at level load time using a prefix which can be registered in LevelObjectCreator::Init and must be set
	*	on the appropriate mesh, yes mesh (blame assimp), in the Level Editor. Or it can be created by registering the create function in s_LevelObjectTypeCreateFunctions
	*	and later calling Level::CreateObject with the appropriate LevelObjectType and pData, this will in turn call LevelObjectCreator::CreateLevelObjectOfType.
	*
	*	Special Objects are similar to ECS::Entities but one Special Object can create many entities.
	*/
	static ELevelObjectType CreateLevelObjectFromPrefix(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static bool CreateLevelObjectOfType(
		ELevelObjectType levelObjectType,
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities);

	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoadDesc>& GetLevelObjectOnLoadDescriptions()
	{
		return s_LevelObjectOnLoadDescriptions;
	}

private:
	static ELevelObjectType CreateNoColliderObject(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreateTeamIndicator(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreatePlayerSpawn(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreatePlayerJail(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreateSpectateMapPoint(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreateFlagSpawn(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreateFlagDeliveryPoint(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static ELevelObjectType CreateKillPlane(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);
	static ELevelObjectType CreateShowerPoint(
		const LambdaEngine::LevelObjectOnLoad& levelObject,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		const glm::vec3& translation);

	static bool CreateFlag(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities);

	static bool CreatePlayer(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities);

	static bool CreatePlayerSpectator(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities);

	static bool CreateProjectile(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<std::tuple<LambdaEngine::String, bool, LambdaEngine::Entity>>>& createdChildEntities);

	static bool FindTeamIndex(const LambdaEngine::String& objectName, uint8& teamIndex);

private:
	inline static bool s_HasDirectionalLight = false;
	inline static LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoadDesc> s_LevelObjectOnLoadDescriptions;
	inline static LambdaEngine::THashTable<LambdaEngine::String, LevelObjectCreateByPrefixFunc> s_LevelObjectByPrefixCreateFunctions;
	inline static LambdaEngine::THashTable<ELevelObjectType, LevelObjectCreateByTypeFunc> s_LevelObjectByTypeCreateFunctions;
};