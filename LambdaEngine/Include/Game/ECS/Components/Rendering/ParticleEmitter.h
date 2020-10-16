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
		DECL_COMPONENT_WITH_DIRTY_FLAG(ParticleEmitterComponent);
		bool			Active			= true;
		bool			OneTime			= false;
		bool			Burst			= false;
		uint32			ParticleCount	= 32U;
		EParticleShape  ParticleShape	= EParticleShape::BILLBOARD;
		GUID_Lambda		MeshID			= 0;
		EEmitterShape	EmitterShape	= EEmitterShape::CONE;
		float			Angle			= 90.f;
		float			Velocity		= 1.0f;
		float			Acceleration	= 0.0f;
		float			LifeTime		= 1.0f;
		float			ParticleRadius	= 1.0f;
	};
}