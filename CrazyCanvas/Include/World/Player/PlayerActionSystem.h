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
	static void ComputeVelocity(const glm::quat& rotation, int8 deltaForward, int8 deltaLeft, glm::vec3& result);

private:
	bool m_MouseEnabled = false;
};