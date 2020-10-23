#pragma once
#include "ECS/System.h"

#include "Types.h"

#include "Math/Math.h"

class NetworkPositionSystem : public LambdaEngine::System
{
public:
	DECL_UNIQUE_CLASS(NetworkPositionSystem);
	NetworkPositionSystem();
	virtual ~NetworkPositionSystem();

	void Init();

private:
	void Tick(LambdaEngine::Timestamp deltaTime) override;

	static void Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage);

private:
	LambdaEngine::IDVector m_Entities;
};