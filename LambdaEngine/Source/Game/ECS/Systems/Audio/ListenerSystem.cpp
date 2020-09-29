#include "Game/ECS/Systems/Audio/ListenerSystem.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Audio/AudioAPI.h"
#include "ECS/ECSCore.h"
#include "Time/API/Clock.h"

namespace LambdaEngine
{
	ListenerSystem ListenerSystem::s_Instance;

	bool ListenerSystem::Init()
	{
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{R, ListenerComponent::Type()}, {R, PositionComponent::Type()}}, {}, &m_ListenerEntities},
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void ListenerSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pListenerComponents = pECS->GetComponentArray<ListenerComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		for (Entity entity : m_ListenerEntities)
		{
			auto& pListenerComponent = pListenerComponents->GetData(entity);
			auto& pPositionComponent = pPositionComponents->GetData(entity);

			pListenerComponent.Desc.Position = pPositionComponent.Position;
			// TODO direction as well
			AudioAPI::GetDevice()->UpdateAudioListener(pListenerComponent.ListenerId, &pListenerComponent.Desc);
		}
	}
}



