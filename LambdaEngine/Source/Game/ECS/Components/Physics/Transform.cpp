#include "Game/ECS/Components/Physics/Transform.h"

#include "Log/Log.h"

#include <cmath>

namespace LambdaEngine
{
    INIT_COMPONENT(PositionComponent);
    INIT_COMPONENT(ScaleComponent);
    INIT_COMPONENT(RotationComponent);
    INIT_COMPONENT(WorldMatrixComponent);

    float GetPitch(const glm::vec3& forward)
    {
        const glm::vec3 right = glm::normalize(glm::cross(forward, g_DefaultUp));
        const glm::vec3 forwardXZ = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
        return glm::orientedAngle(forwardXZ, forward, right);
    }

    float GetYaw(const glm::vec3& forward)
    {
        const glm::vec3 forwardXZ = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
        return glm::orientedAngle(g_DefaultForward, forwardXZ, g_DefaultUp);
    }

    void RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle)
    {
        V = V - P;
        V = glm::rotate(glm::angleAxis(angle, axis), V);
        V = V + P;
    }
}
