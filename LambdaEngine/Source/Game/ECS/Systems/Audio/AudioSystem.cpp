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
			SetComponentOwner<AudibleComponent>({ .Destructor = &AudioSystem::AudibleComponentDestructor });

			RegisterSystem(TYPE_NAME(AudioSystem), systemReg);
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
		ComponentArray<ListenerComponent>* pListenerComponents			= pECS->GetComponentArray<ListenerComponent>();

		for (Entity entity : m_AudibleEntities)
		{
			auto& audibleComponent = pAudibleComponents->GetData(entity);
			auto& positionComponent = pPositionComponents->GetConstData(entity);

			for (auto soundInstancePair : audibleComponent.SoundInstances3D)
			{
				soundInstancePair.second->SetPosition(positionComponent.Position);
			}
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
	}

	void AudioSystem::AudibleComponentDestructor(AudibleComponent& audibleComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		for (auto soundInstancePair : audibleComponent.SoundInstances3D)
		{
			SAFEDELETE(soundInstancePair.second);
		}

		for (auto soundInstancePair : audibleComponent.SoundInstances2D)
		{
			SAFEDELETE(soundInstancePair.second);
		}

		audibleComponent.SoundInstances3D.clear();
		audibleComponent.SoundInstances2D.clear();
	}
}
