#pragma once

#include "ECS/System.h"
#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Audio/ListenerComponent.h"

namespace LambdaEngine
{
	class AudioSystem : public System, public ComponentOwner
	{
	public:
		DECL_UNIQUE_CLASS(AudioSystem);
		~AudioSystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

	public:
		static AudioSystem& GetInstance() { return s_Instance; }

	private:
		AudioSystem() = default;
		void AudibleComponentDestructor(AudibleComponent& audibleComponent);

	private:
		IDVector	m_AudibleEntities;
		IDVector	m_ListenerEntities;
		IDVector	m_AudibleNoPositionEntities;
		IDVector	m_CameraEntities;
	private:
		static AudioSystem s_Instance;
	};
}
