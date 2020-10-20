#include "ECS/ECSCore.h"
#include "Game/ECS/Systems/Audio/AudioSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Input/API/InputActionSystem.h"
#include "Audio/AudioAPI.h"
#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	AudioSystem AudioSystem::s_Instance;

	bool AudioSystem::Init()
	{
		{
			TransformGroup transformGroup;
			transformGroup.Position.Permissions	= R;
			transformGroup.Rotation.Permissions	= R;

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_CameraEntities,
					.ComponentAccesses =
					{
						{R, AudibleComponent::Type()}, {R, PositionComponent::Type()}, {R, CameraComponent::Type()}
					}
				},
				{
					.pSubscriber = &m_AudibleEntities,
					.ComponentAccesses =
					{
						{R, AudibleComponent::Type()}, {R, PositionComponent::Type()}
					}
				},
				{
					.pSubscriber = &m_ListenerEntities,
					.ComponentAccesses =
					{
						{RW, ListenerComponent::Type()}
					},
					.ComponentGroups =
					{
						&transformGroup
					}
				}
			};
			systemReg.Phase = 0;
			SetComponentOwner<AudibleComponent>({ std::bind_front(&AudioSystem::AudibleComponentDestructor, this) });

			RegisterSystem(systemReg);
		}

		return true;
	}

	void AudioSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		ECSCore* pECS = ECSCore::GetInstance();

		ComponentArray<AudibleComponent>* pAudibleComponents		= pECS->GetComponentArray<AudibleComponent>();
		const ComponentArray<PositionComponent>* pPositionComponents	= pECS->GetComponentArray<PositionComponent>();
		const ComponentArray<RotationComponent>* pRotationComponents	= pECS->GetComponentArray<RotationComponent>();
		const ComponentArray<CameraComponent>* pCameraComponents		= pECS->GetComponentArray<CameraComponent>();
		ComponentArray<ListenerComponent>* pListenerComponents			= pECS->GetComponentArray<ListenerComponent>();

		for (Entity entity : m_AudibleEntities)
		{
			auto& audibleComponent = pAudibleComponents->GetData(entity);
			auto& positionComponent = pPositionComponents->GetConstData(entity);

			auto* pSoundInstance = audibleComponent.pSoundInstance;
			pSoundInstance->SetPosition(positionComponent.Position);

			pSoundInstance->Play();

		}

		for (Entity entity : m_ListenerEntities)
		{
			auto& pListenerComponent = pListenerComponents->GetData(entity);
			auto& pPositionComponent = pPositionComponents->GetConstData(entity);
			auto& pRotationComponent = pRotationComponents->GetConstData(entity);

			pListenerComponent.Desc.Position = pPositionComponent.Position;
			pListenerComponent.Desc.Forward = GetForward(pRotationComponent.Quaternion);

			AudioAPI::GetDevice()->UpdateAudioListener(pListenerComponent.ListenerId, &pListenerComponent.Desc);
		}

		for (Entity entity : m_CameraEntities)
		{
			auto& audibleComponent = pAudibleComponents->GetData(entity);
			auto& positionComponent = pPositionComponents->GetConstData(entity);
			auto& cameraComponent = pCameraComponents->GetConstData(entity);

			auto* pSoundInstance = audibleComponent.pSoundInstance;
			pSoundInstance->SetPosition(positionComponent.Position);

			bool isMoving = InputActionSystem::IsActive("CAM_FORWARD") ||
				InputActionSystem::IsActive("CAM_LEFT") ||
				InputActionSystem::IsActive("CAM_BACKWARD") ||
				InputActionSystem::IsActive("CAM_RIGHT");

			if (cameraComponent.IsActive && isMoving)
			{
				pSoundInstance->Play();
			}
			else
			{
				pSoundInstance->Pause();
			}
		}
	}

	void AudioSystem::AudibleComponentDestructor(AudibleComponent& audibleComponent)
	{
		SAFEDELETE(audibleComponent.pSoundInstance);

	}
}



