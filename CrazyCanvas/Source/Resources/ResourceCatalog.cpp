#include "Resources/ResourceCatalog.h"

#include "Resources/ResourceManager.h"

#include "Game/ECS/Components/Rendering/AnimationComponent.h"

bool ResourceCatalog::Init()
{
	using namespace LambdaEngine;

	//Music
	{
		MAIN_MENU_MUSIC_GUID = ResourceManager::LoadMusicFromFile("MainMenuCC.mp3");
	}

	//Paint
	{
		BRUSH_MASK_GUID = ResourceManager::LoadTextureFromFile("MeshPainting/BrushMaskV3.png", EFormat::FORMAT_R8G8B8A8_UNORM, false, false);
	}

	//Flag
	{
		ResourceManager::LoadMeshAndMaterialFromFile("Roller.glb", FLAG_MESH_GUID, FLAG_COMMON_MATERIAL_GUID);
	}

	//Player
	{
		ResourceManager::LoadMeshFromFile("Player/Idle.glb", PLAYER_MESH_GUID, PLAYER_IDLE_GUIDs);

#ifdef USE_ALL_ANIMATIONS
		PLAYER_RUN_GUIDs					= ResourceManager::LoadAnimationsFromFile("Player/Run.glb");
		PLAYER_RUN_MIRRORED_GUIDs			= ResourceManager::LoadAnimationsFromFile("Player/RunMirrored.glb");
		PLAYER_RUN_BACKWARD_GUIDs			= ResourceManager::LoadAnimationsFromFile("Player/RunBackward.glb");
		PLAYER_RUN_BACKWARD_MIRRORED_GUIDs	= ResourceManager::LoadAnimationsFromFile("Player/RunBackwardMirrored.glb");
		PLAYER_STRAFE_LEFT_GUIDs			= ResourceManager::LoadAnimationsFromFile("Player/StrafeLeft.glb");
		PLAYER_STRAFE_RIGHT_GUIDs			= ResourceManager::LoadAnimationsFromFile("Player/StrafeRight.glb");
#endif

		PLAYER_STEP_SOUND_GUID = ResourceManager::LoadSoundEffect3DFromFile("Player/step.wav");
		PLAYER_DEATH_SOUND_GUID = ResourceManager::LoadSoundEffect3DFromFile("Player/DeathSound.mp3");
	}

	// Projectile
	{
		ResourceManager::LoadMeshFromFile("sphere.obj", PROJECTILE_MESH_GUID);

		MaterialProperties projectileMaterialProperties;
		projectileMaterialProperties.Metallic = 0.8f;
		projectileMaterialProperties.Roughness = 0.1f;
		projectileMaterialProperties.Albedo = glm::vec4(0.34, 0.85, 1.0f, 1.0f);

		PROJECTILE_WATER_MATERIAL = ResourceManager::LoadMaterialFromMemory(
			"Water Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);
	}

	//Weapon
	{
		ResourceManager::LoadMeshAndMaterialFromFile("Gun/Gun.glb", WEAPON_MESH_GUID, WEAPON_MATERIAL_GUID);
		ResourceManager::LoadMeshFromFile("Gun/GunFirstPerson.glb", WEAPON_FIRST_PERSON_MESH_GUID);

		// Soundeffects
		WEAPON_SOUND_GUNFIRE_2D_GUID = ResourceManager::LoadSoundEffect2DFromFile("Weapon/Shootsound.mp3");
		WEAPON_SOUND_GUNFIRE_3D_GUID = ResourceManager::LoadSoundEffect3DFromFile("Weapon/Shootsound.mp3");
		WEAPON_SOUND_OUTOFAMMO_2D_GUID = ResourceManager::LoadSoundEffect2DFromFile("Weapon/WaterSound.mp3");
	}

	// General Sound Effects
	{
		SOUND_EFFECT_SPLASH0_3D_GUID = ResourceManager::LoadSoundEffect3DFromFile("Player/HitSound0.mp3");
		SOUND_EFFECT_SPLASH1_2D_GUID = ResourceManager::LoadSoundEffect2DFromFile("Player/HitSound1.mp3");
	}

	return true;
}
