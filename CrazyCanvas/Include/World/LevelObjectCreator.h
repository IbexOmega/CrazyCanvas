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
	LEVEL_OBJECT_TYPE_NONE					= 0,
	LEVEL_OBJECT_TYPE_STATIC_GEOMTRY		= 1,
	LEVEL_OBJECT_TYPE_DIR_LIGHT				= 2,
	LEVEL_OBJECT_TYPE_POINT_LIGHT			= 3,
	LEVEL_OBJECT_TYPE_PLAYER_SPAWN			= 4,
	LEVEL_OBJECT_TYPE_PLAYER				= 5,
	LEVEL_OBJECT_TYPE_FLAG_SPAWN			= 6,
	LEVEL_OBJECT_TYPE_FLAG					= 7,
	LEVEL_OBJECT_TYPE_FLAG_DELIVERY_POINT	= 8,
	LEVEL_OBJECT_TYPE_PROJECTILE			= 9,
	LEVEL_OBJECT_TYPE_KILL_PLANE			= 10,
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
	uint32		TeamIndex;
	LambdaEngine::Entity			WeaponOwner;
	LambdaEngine::CollisionCallback	Callback;
	LambdaEngine::MeshComponent		MeshComponent;
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
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>&);

	static constexpr const float PLAYER_CAPSULE_HEIGHT = 1.8f;
	static constexpr const float PLAYER_CAPSULE_RADIUS = 0.2f;

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
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities);

	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoadDesc>& GetLevelObjectOnLoadDescriptions()
	{
		return s_LevelObjectOnLoadDescriptions;
	}

private:
	static ELevelObjectType CreatePlayerSpawn(
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

	static bool CreateFlag(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities);

	static bool CreatePlayer(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities);

	static bool CreateProjectile(
		const void* pData,
		LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
		LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities);

	static bool FindTeamIndex(const LambdaEngine::String& objectName, uint8& teamIndex);

private:
	inline static LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoadDesc> s_LevelObjectOnLoadDescriptions;
	inline static LambdaEngine::THashTable<LambdaEngine::String, LevelObjectCreateByPrefixFunc> s_LevelObjectByPrefixCreateFunctions;
	inline static LambdaEngine::THashTable<ELevelObjectType, LevelObjectCreateByTypeFunc> s_LevelObjectByTypeCreateFunctions;

	inline static GUID_Lambda s_FlagMeshGUID		= GUID_NONE;
	inline static GUID_Lambda s_FlagCommonMaterialGUID	= GUID_NONE;

	inline static GUID_Lambda s_PlayerMeshGUID = GUID_NONE;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerIdleGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerRunGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerRunMirroredGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerRunBackwardGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerRunBackwardMirroredGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerStrafeLeftGUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> s_PlayerStrafeRightGUIDs;
	inline static GUID_Lambda s_PlayerStepSoundGUID = GUID_NONE;

	inline static GUID_Lambda s_WeaponMeshGUID = GUID_NONE;
	inline static GUID_Lambda s_WeaponMaterialGUID = GUID_NONE;
};