#pragma once

#include "LambdaEngine.h"

constexpr const float32 GRAVITATIONAL_ACCELERATION		= -9.81f;

constexpr const float32 PLAYER_FALLING_MULTIPLIER		= 1.0f;
constexpr const float32 PLAYER_FRICTION					= 20.0f;
constexpr const float32 PLAYER_ACCELERATION_AIR			= 2.5f;
constexpr const float32 PLAYER_ACCELERATION_GROUND		= 150.0f;
constexpr const float32 PLAYER_MAX_RUN_VELOCITY_GROUND	= 5.0f;
constexpr const float32 PLAYER_MAX_WALK_VELOCITY_GROUND	= 2.5f;
constexpr const float32 PLAYER_MAX_VELOCITY_AIR			= 5.0f;


//constexpr const float32 PLAYER_WALK_MOVEMENT_SPEED	= 2.5f;
//constexpr const float32 PLAYER_RUN_MOVEMENT_SPEED	= 5.0f;

constexpr const float32 PLAYER_DRAG					= 15.0f;
constexpr const float32 PLAYER_JUMP_SPEED			= 5.0f;