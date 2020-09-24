#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"

namespace LambdaEngine
{
	class AudioSystem : public System
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


	private:
		IDVector	m_AudibleEntities;
		IDVector	m_AudibleNoPositionEntities;

	private:
		static AudioSystem s_Instance;
	};
}