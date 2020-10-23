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
		TUBE = 2,
	};

	struct ParticleEmitterComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(ParticleEmitterComponent);
		bool			Active				= true;
		bool			OneTime				= false;
		float			Explosive			= 0.f;
		float			SpawnSpeed			= 0.1f; // Only used if explosive is false.
		uint32			ParticleCount		= 32U;
		EParticleShape  ParticleShape		= EParticleShape::BILLBOARD;
		GUID_Lambda		MeshID				= 0;
		EEmitterShape	EmitterShape		= EEmitterShape::CONE;
		float			Angle				= 90.f;
		float			Velocity			= 1.0f;
		float			Acceleration		= 0.0f;
		float			Gravity				= -9.81f;
		float			LifeTime			= 1.0f;
		float			BeginRadius			= 1.0f;
		float			EndRadius			= 0.0f;
		GUID_Lambda		AtlasGUID			= GUID_NONE;
		uint32			AtlasTileSize		= 64;
		uint32			TileIndex			= 0;
		uint32			AnimationCount		= 1;
		uint32			FirstAnimationIndex = 0;
		uint32			LastAnimationIndex	= 0;
		glm::vec4		Color				= {1.f, 1.f, 1.f, 1.f};
	};
}