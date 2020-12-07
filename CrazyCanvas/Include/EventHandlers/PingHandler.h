#pragma once

#include "Containers/IDVector.h"
#include "ECS/EntitySubscriber.h"

#define MAX_PING_DISTANCE 100.0f
#define PING_DURATION 8.0f // Seconds until a ping is removed (unless it is replaced with a new ping)

namespace LambdaEngine
{
	struct KeyPressedEvent;
}

class PingHandler : LambdaEngine::EntitySubscriber
{
public:
	PingHandler() = default;
	~PingHandler();

	void Init();

private:
	bool OnKeyPress(const LambdaEngine::KeyPressedEvent& keyPressEvent);

private:
	LambdaEngine::IDVector m_LocalPlayerCamera;
};
