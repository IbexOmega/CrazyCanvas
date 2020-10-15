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
		uint32			ParticleCount	= 32U;
		EParticleShape  ParticleShape	= EParticleShape::BILLBOARD;
		GUID_Lambda		MeshID			= 0;
		EEmitterShape	EmitterShape	= EEmitterShape::CONE;
		float			Angle			= 90.f;
		float			Velocity		= 1.0f;
		float			Acceleration	= 0.0f;
		float			LifeTime		= 1.0f;
		float			ParticleRadius	= 1.0f;
		GUID_Lambda		AtlasGUID		= GUID_NONE;
		uint32			AtlasTileSize	= 64;
		uint32			TextureIndex	= 0;
		uint32			AnimationCount	= 1;
	};
}