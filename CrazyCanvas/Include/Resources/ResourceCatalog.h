#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"

class ResourceCatalog
{
public:
	static bool Init();

public:
	//Music
	inline static GUID_Lambda MAIN_MENU_MUSIC_GUID = GUID_NONE;

	//Paint
	inline static GUID_Lambda BRUSH_MASK_GUID = GUID_NONE;

	//Flag
	inline static GUID_Lambda FLAG_MESH_GUID = GUID_NONE;
	inline static GUID_Lambda FLAG_COMMON_MATERIAL_GUID = GUID_NONE;

	//Player
	inline static GUID_Lambda PLAYER_MESH_GUID = GUID_NONE;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_IDLE_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_RUN_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_RUN_MIRRORED_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_RUN_BACKWARD_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_RUN_BACKWARD_MIRRORED_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_STRAFE_LEFT_GUIDs;
	inline static LambdaEngine::TArray<GUID_Lambda> PLAYER_STRAFE_RIGHT_GUIDs;
	inline static GUID_Lambda PLAYER_STEP_SOUND_GUID = GUID_NONE;

	//Weapon
	inline static GUID_Lambda WEAPON_MESH_GUID = GUID_NONE;
	inline static GUID_Lambda WEAPON_MATERIAL_GUID = GUID_NONE;
};