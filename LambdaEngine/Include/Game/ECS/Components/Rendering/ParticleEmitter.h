#pragma once


#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	enum EParticleShape
	{
		BILLBOARD	= 0,
		MESH		= 1,
	};

	enum EEmitterShape
	{
		CONE = 0,
		SPHERE = 1,
	};

	struct ParticleEmitterComponent
	{
		DECL_COMPONENT(ParticleEmitterComponent);
		bool			OneTime			= false;
		bool			Burst			= false;
		uint32			ParticleCount	= 10U;
		EParticleShape  ParticleShape	= EParticleShape::BILLBOARD;
		GUID_Lambda		MeshID			= 0;
		EEmitterShape	EmitterShape	= EEmitterShape::CONE;
		float			Angle			= 90.f;
		glm::vec3		Velocity		= glm::vec3(0.0f);
		glm::vec3		Acceleration	= glm::vec3(0.0f);
		float			LifeTime		= 1.0f;
		float			ParticleRadius	= 1.0f;
	};
}