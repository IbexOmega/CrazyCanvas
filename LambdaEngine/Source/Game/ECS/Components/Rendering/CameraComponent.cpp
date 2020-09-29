#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Audio/ListenerComponent.h"
#include "Audio/AudioAPI.h"

#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Containers/TSharedPtr.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "ECS/ECSCore.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	Entity CreateFreeCameraEntity(const CameraDesc& cameraDesc)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		FreeCameraComponent freeCamComp;
		pECS->AddComponent<FreeCameraComponent>(entity, freeCamComp);

		ViewProjectionMatricesComponent viewProjComp;
		viewProjComp.Projection = glm::perspective(glm::radians(cameraDesc.FOVDegrees), cameraDesc.Width / cameraDesc.Height, cameraDesc.NearPlane, cameraDesc.FarPlane);
		viewProjComp.View = glm::lookAt(cameraDesc.Position, cameraDesc.Position + cameraDesc.Direction, g_DefaultUp);
		pECS->AddComponent<ViewProjectionMatricesComponent>(entity, viewProjComp);

		CameraComponent camComp;
		camComp.FarPlane 	= cameraDesc.FarPlane;
		camComp.NearPlane 	= cameraDesc.NearPlane;
		camComp.FOV			= cameraDesc.FOVDegrees;
		pECS->AddComponent<CameraComponent>(entity, camComp);

		pECS->AddComponent<PositionComponent>(entity, PositionComponent{ .Position = cameraDesc.Position });
		pECS->AddComponent<RotationComponent>(entity, RotationComponent{ .Quaternion = glm::identity<glm::quat>() });
		pECS->AddComponent<ScaleComponent>(entity, ScaleComponent{ .Scale = {1.f, 1.f, 1.f} });

		// Audio Footsteps
		GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("walking-short.wav");
		ISoundEffect3D* SoundEffect = ResourceManager::GetSoundEffect(soundGUID);
		ISoundInstance3D* SoundInstance = new SoundInstance3DFMOD(AudioAPI::GetDevice());
		const SoundInstance3DDesc desc = {
				.pName = "SoundInstance3DFMOD",
				.pSoundEffect = SoundEffect,
				.Flags = FSoundModeFlags::SOUND_MODE_NONE
		};
		float volume = 0.1f;
		SoundInstance->Init(&desc);
		SoundInstance->SetVolume(volume);
		pECS->AddComponent<AudibleComponent>(entity, { SoundEffect, SoundInstance });

		// Listener
		pECS->AddComponent<ListenerComponent>(entity, { AudioAPI::GetDevice()->CreateAudioListener(), {} });

		return entity;
	}

	Entity CreateCameraTrackEntity(const LambdaEngine::CameraDesc& cameraDesc, const TArray<glm::vec3>& track)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		TrackComponent camTrackComp;
		camTrackComp.Track = track;
		pECS->AddComponent<TrackComponent>(entity, camTrackComp);

		ViewProjectionMatricesComponent viewProjComp;
		viewProjComp.Projection = glm::perspective(glm::radians(cameraDesc.FOVDegrees), cameraDesc.Width / cameraDesc.Height, cameraDesc.NearPlane, cameraDesc.FarPlane);
		viewProjComp.View = glm::lookAt(cameraDesc.Position, cameraDesc.Position + cameraDesc.Direction, g_DefaultUp);
		pECS->AddComponent<ViewProjectionMatricesComponent>(entity, viewProjComp);

		CameraComponent camComp;
		camComp.FarPlane = cameraDesc.FarPlane;
		camComp.NearPlane = cameraDesc.NearPlane;
		pECS->AddComponent<CameraComponent>(entity, camComp);

		pECS->AddComponent<PositionComponent>(entity, PositionComponent{ .Position = cameraDesc.Position });
		pECS->AddComponent<RotationComponent>(entity, RotationComponent{ .Quaternion = glm::quatLookAtLH(cameraDesc.Direction, g_DefaultUp) });
		pECS->AddComponent<ScaleComponent>(entity, ScaleComponent{ .Scale = {1.f, 1.f, 1.f} });

		return entity;
	}
}
