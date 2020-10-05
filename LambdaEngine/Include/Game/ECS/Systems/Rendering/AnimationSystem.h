#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Resources/Mesh.h"

#include "Time/API/Clock.h"

namespace LambdaEngine
{
	struct MeshComponent;
	struct AnimationComponent;

	class AnimationSystem : public System
	{
	public:
		bool Init();

		// Inherited from system
		virtual void Tick(Timestamp deltaTime) override final;

	private:
		AnimationSystem();
		~AnimationSystem();

		void InitClock();

		void Animate(float64 seconds, AnimationComponent& animation, MeshComponent& mesh);
		glm::mat4 ApplyParent(Joint& bone, Skeleton& skeleton, TArray<glm::mat4>& matrices);

	public:
		static AnimationSystem& GetInstance();

	private:
		bool		m_HasInitClock = false;
		Clock		m_Clock;
		IDVector	m_AnimationEntities;
	};
}