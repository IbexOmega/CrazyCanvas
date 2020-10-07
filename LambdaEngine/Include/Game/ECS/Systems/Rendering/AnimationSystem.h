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

		FORCEINLINE float64 GetTotalTimeInSeconds() const
		{
			return m_HasInitClock ? m_Clock.GetTotalTime().AsSeconds() : 0.0;
		}

	private:
		AnimationSystem();
		~AnimationSystem();

		void Animate(AnimationComponent& animation);
		glm::mat4 ApplyParent(const Joint& bone, Skeleton& skeleton, TArray<glm::mat4>& matrices);

		void OnEntityAdded(Entity entity);

	public:
		static AnimationSystem& GetInstance();

	private:
		bool		m_HasInitClock = false;
		Clock		m_Clock;
		IDVector	m_AnimationEntities;
	};
}
