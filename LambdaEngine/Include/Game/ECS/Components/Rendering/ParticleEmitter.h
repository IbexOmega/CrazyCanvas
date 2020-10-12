#pragma once


#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{


	struct ParticleEmitterComponent
	{
		enum Shape
		{
			CONE = 0,
			SPHERE = 0,
		};

		DECL_COMPONENT(ParticleEmitterComponent);
		bool		Burst			= false;
		uint32		ParticleCount	= 10U;
		Shape		Shape			= Shape::CONE;
		float		Angle			= 90.f;
		glm::vec3	Velocity		= glm::vec3(0.0f);
		glm::vec3	Acceleration	= glm::vec3(0.0f);
		float		LifeTime		= 1.0f;
		float		ParticleRadius	= 1.0f;
	};
}