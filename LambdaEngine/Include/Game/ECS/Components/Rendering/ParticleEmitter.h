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
		/*
		*	 Determines if emitter is active
		*/
		bool			Active					= true;
		/*
		*	 Emitter will emit once until active is set to true again
		*/
		bool			OneTime					= false;
		/*
		*	 0 equals an explosion and 1 will spawn particles sequentially using spawn speed as delay offset, should be between 0.0 - 1.0
		*/
		float			Explosive				= 0.f;
		/*
		*	The rate at which particles will be spawned 
		*/
		float			SpawnDelay				= 0.1f;
		uint32			ParticleCount			= 32U;
		EParticleShape  ParticleShape			= EParticleShape::BILLBOARD;
		/*
		*	Not implemented yet
		*/
		GUID_Lambda		MeshID					= 0;
		/*
		*	The shape that particles will spawn upon
		*/
		EEmitterShape	EmitterShape			= EEmitterShape::CONE;
		/*
		*	Used for cone shap to determine arc of cone
		*/
		float			Angle					= 90.f;
		/*
		* Randomness factor for particle velocity, should be between 0.0 - 1.0
		*/
		float			VelocityRandomness		= 0.0f;
		float			Velocity				= 1.0f;
		/*
		* Randomness factor for particle acceleration, should be between 0.0 - 1.0
		*/
		float			AccelerationRandomness	= 0.0f;
		float			Acceleration			= 0.0f;
		float			Gravity					= -9.81f;
		float			LifeTime				= 1.0f;
		/*
		* Randomness factor for particle radius, should be between 0.0 - 1.0
		*/
		float			RadiusRandomness		= 0.0f;
		float			BeginRadius				= 1.0f;
		float			EndRadius				= 0.0f;
		/*
		* This will be multiplied by the velocity, meaning that, if it is 1, no friction is applied, if 0, full friction.
		*/
		float			FrictionFactor			= 0.9f;
		/*
		* How much of an opposite force the particles should have, 1 is the standard force (no bounciness), if greater than 1, it will bounce, if lower it will fall throuh the ground.
		*/
		float			Bounciness				= 1.0f;
		GUID_Lambda		AtlasGUID				= GUID_NONE;
		uint32			AtlasTileSize			= 64;
		bool			RandomStartIndex		= false;
		uint32			AnimationCount			= 1;
		uint32			FirstAnimationIndex		= 0;
		uint32			LastAnimationIndex		= 0;
		glm::vec4		Color					= {1.f, 1.f, 1.f, 1.f};
	};
}