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
			TransformComponents transformComponents;
			transformComponents.Position.Permissions	= R;
			transformComponents.Scale.Permissions		= NDA;
			transformComponents.Rotation.Permissions	= R;

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
						&transformComponents
					}
				}
			};
			systemReg.Phase = 0;
			SetComponentOwner<AudibleComponent>({ std::bind(&AudioSystem::AudibleComponentDestructor, this, std::placeholders::_1) });

			RegisterSystem(systemReg);
		}

		return true;
	}

	void AudioSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pAudibleComponents = pECS->GetComponentArray<AudibleComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		auto* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		auto* pCameraComponents = pECS->GetComponentArray<CameraComponent>();
		auto* pListenerComponents = pECS->GetComponentArray<ListenerComponent>();

		for (Entity entity : m_AudibleEntities)
		{
			auto& audibleComponent = pAudibleComponents->GetData(entity);
			auto& positionComponent = pPositionComponents->GetData(entity);

			auto* pSoundInstance = audibleComponent.pSoundInstance;
			pSoundInstance->SetPosition(positionComponent.Position);

			pSoundInstance->Play();

		}

		for (Entity entity : m_ListenerEntities)
		{
			auto& pListenerComponent = pListenerComponents->GetData(entity);
			auto& pPositionComponent = pPositionComponents->GetData(entity);
			auto& pRotationComponent = pRotationComponents->GetData(entity);

			pListenerComponent.Desc.Position = pPositionComponent.Position;
			pListenerComponent.Desc.Forward = GetForward(pRotationComponent.Quaternion);

			AudioAPI::GetDevice()->UpdateAudioListener(pListenerComponent.ListenerId, &pListenerComponent.Desc);
		}

		for (Entity entity : m_CameraEntities)
		{
			auto& audibleComponent = pAudibleComponents->GetData(entity);
			auto& positionComponent = pPositionComponents->GetData(entity);
			auto& cameraComponent = pCameraComponents->GetData(entity);

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



