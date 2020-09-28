#include "Game/ECS/Systems/Audio/AudioSystem.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Audio/AudioAPI.h"
#include "ECS/ECSCore.h"
#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	AudioSystem AudioSystem::s_Instance;

	bool AudioSystem::Init()
	{
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{R, AudibleComponent::Type()}, {R, PositionComponent::Type()}, {R, CameraComponent::Type()}}, {}, &m_CameraEntities},
				{{{R, AudibleComponent::Type()}, {R, PositionComponent::Type()}},								{},	&m_AudibleEntities},
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void AudioSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pAudibleComponents =	pECS->GetComponentArray<AudibleComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		auto* pCameraComponents =	pECS->GetComponentArray<CameraComponent>();

		PositionComponent* pActiveCamPosComp = nullptr;

		for (Entity entity : m_CameraEntities)
		{
			auto& audibleComponent		=		pAudibleComponents->GetData(entity);
			auto& positionComponent		=		pPositionComponents->GetData(entity);
			auto& cameraComponent		=		pCameraComponents->GetData(entity);

			auto* pSoundInstance		=		audibleComponent.pSoundInstance.Get();

			if (cameraComponent.IsActive)
			{
				pSoundInstance->Play();
				LOG_MESSAGE("%d is playing now", entity);
			}
		}
	}
}
	


