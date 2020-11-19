#pragma once

#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/KeyEvents.h"

class PlayerActionSystem
{
public:
	DECL_UNIQUE_CLASS(PlayerActionSystem);
	PlayerActionSystem();
	virtual ~PlayerActionSystem();

	void Init();

	void TickMainThread(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entityPlayer);

private:
	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

public:
	static void ComputeVelocity(const glm::quat& rotation, const glm::i8vec3& deltaAction, bool walking, float32 dt, glm::vec3& velocity);
	static void SetMouseEnabled(bool isEnabled);

private:
	static bool m_MouseEnabled;
};